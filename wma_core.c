/* Audivra WMA helper.
 * Uses FFmpeg (LGPL 2.1+) libraries built with ONLY the ASF reader and the WMA decoders.
 * See README.md for the license notice and build recipe. */
#include "wma_core.h"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/mem.h>
#include <libswresample/swresample.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void put16(uint8_t *p, uint16_t v) { p[0] = v & 255; p[1] = v >> 8; }
static void put32(uint8_t *p, uint32_t v) {
    p[0] = v & 255; p[1] = (v >> 8) & 255; p[2] = (v >> 16) & 255; p[3] = v >> 24;
}

static void make_header(uint8_t h[44], int channels, int rate, uint32_t data_bytes) {
    memcpy(h, "RIFF", 4);
    put32(h + 4, 36 + data_bytes);
    memcpy(h + 8, "WAVEfmt ", 8);
    put32(h + 16, 16);
    put16(h + 20, 1); /* PCM */
    put16(h + 22, (uint16_t)channels);
    put32(h + 24, (uint32_t)rate);
    put32(h + 28, (uint32_t)(rate * channels * 2));
    put16(h + 32, (uint16_t)(channels * 2));
    put16(h + 34, 16);
    memcpy(h + 36, "data", 4);
    put32(h + 40, data_bytes);
}

/* Converts one decoded frame (or NULL to flush) and appends it to the file. */
static int write_converted(SwrContext *swr, const AVFrame *frame, int out_channels,
                           uint8_t **buf, int *buf_samples, FILE *out, uint64_t *total) {
    int in_samples = frame ? frame->nb_samples : 0;
    int max_out = swr_get_out_samples(swr, in_samples);
    if (max_out < 0) return max_out;
    if (max_out > *buf_samples) {
        av_freep(buf);
        *buf = av_malloc((size_t)max_out * out_channels * 2);
        if (!*buf) return AVERROR(ENOMEM);
        *buf_samples = max_out;
    }
    int got = swr_convert(swr, buf, max_out,
                          frame ? (const uint8_t **)frame->extended_data : NULL, in_samples);
    if (got < 0) return got;
    size_t bytes = (size_t)got * out_channels * 2;
    if (bytes && fwrite(*buf, 1, bytes, out) != bytes) return -1;
    *total += bytes;
    return 0;
}

int wma_decode_to_wav(const char *in_path, const char *out_path) {
    int ret = -1;
    AVFormatContext *fmt = NULL;
    AVCodecContext *ctx = NULL;
    SwrContext *swr = NULL;
    AVPacket *pkt = NULL;
    AVFrame *frame = NULL;
    FILE *out = NULL;
    uint8_t *buf = NULL;
    int buf_samples = 0;
    uint64_t total = 0;

    av_log_set_level(AV_LOG_QUIET);

    if (avformat_open_input(&fmt, in_path, NULL, NULL) < 0) { ret = -2; goto done; }
    if (avformat_find_stream_info(fmt, NULL) < 0) { ret = -3; goto done; }
    int si = av_find_best_stream(fmt, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (si < 0) { ret = -4; goto done; }
    AVStream *st = fmt->streams[si];
    const AVCodec *dec = avcodec_find_decoder(st->codecpar->codec_id);
    if (!dec) { ret = -5; goto done; }
    ctx = avcodec_alloc_context3(dec);
    if (!ctx || avcodec_parameters_to_context(ctx, st->codecpar) < 0) { ret = -6; goto done; }
    if (avcodec_open2(ctx, dec, NULL) < 0) { ret = -7; goto done; }

    int in_channels = ctx->ch_layout.nb_channels;
    if (in_channels <= 0) { ret = -8; goto done; }
    AVChannelLayout in_layout;
    if (ctx->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC) {
        av_channel_layout_default(&in_layout, in_channels);
    } else {
        av_channel_layout_copy(&in_layout, &ctx->ch_layout);
    }
    int out_channels = in_channels >= 2 ? 2 : 1;
    AVChannelLayout out_layout;
    av_channel_layout_default(&out_layout, out_channels);
    int rate = ctx->sample_rate;

    if (swr_alloc_set_opts2(&swr, &out_layout, AV_SAMPLE_FMT_S16, rate,
                            &in_layout, ctx->sample_fmt, rate, 0, NULL) < 0 || swr_init(swr) < 0) {
        av_channel_layout_uninit(&in_layout);
        av_channel_layout_uninit(&out_layout);
        ret = -9;
        goto done;
    }
    av_channel_layout_uninit(&in_layout);
    av_channel_layout_uninit(&out_layout);

    out = fopen(out_path, "wb");
    if (!out) { ret = -10; goto done; }
    uint8_t header[44];
    make_header(header, out_channels, rate, 0);
    if (fwrite(header, 1, 44, out) != 44) { ret = -11; goto done; }

    pkt = av_packet_alloc();
    frame = av_frame_alloc();
    if (!pkt || !frame) { ret = -12; goto done; }

    int draining = 0;
    for (;;) {
        if (!draining) {
            int r = av_read_frame(fmt, pkt);
            if (r < 0) {
                avcodec_send_packet(ctx, NULL);
                draining = 1;
            } else {
                if (pkt->stream_index == si) avcodec_send_packet(ctx, pkt);
                av_packet_unref(pkt);
            }
        }
        for (;;) {
            int r = avcodec_receive_frame(ctx, frame);
            if (r == AVERROR(EAGAIN) || r == AVERROR_EOF) break;
            if (r < 0) break; /* a damaged frame: skip it and keep going */
            int w = write_converted(swr, frame, out_channels, &buf, &buf_samples, out, &total);
            av_frame_unref(frame);
            if (w < 0) { ret = -13; goto done; }
        }
        if (draining) break;
    }
    /* flush whatever the resampler still holds */
    if (write_converted(swr, NULL, out_channels, &buf, &buf_samples, out, &total) < 0) {
        ret = -14;
        goto done;
    }
    if (total == 0) { ret = -15; goto done; } /* nothing decoded (protected or unsupported file) */

    make_header(header, out_channels, rate, (uint32_t)(total > 0xFFFFFFF0u ? 0xFFFFFFF0u : total));
    fflush(out);
    if (fseek(out, 0, SEEK_SET) != 0 || fwrite(header, 1, 44, out) != 44) { ret = -16; goto done; }
    ret = 0;

done:
    if (out) fclose(out);
    if (ret != 0 && out_path) remove(out_path);
    av_freep(&buf);
    av_frame_free(&frame);
    av_packet_free(&pkt);
    swr_free(&swr);
    avcodec_free_context(&ctx);
    avformat_close_input(&fmt);
    return ret;
}

static const char *tag(AVFormatContext *fmt, const char *key) {
    const AVDictionaryEntry *e = av_dict_get(fmt->metadata, key, NULL, 0);
    return e && e->value ? e->value : "";
}

int wma_probe(const char *in_path, char *out, int out_size) {
    AVFormatContext *fmt = NULL;
    av_log_set_level(AV_LOG_QUIET);
    if (avformat_open_input(&fmt, in_path, NULL, NULL) < 0) return -2;
    if (avformat_find_stream_info(fmt, NULL) < 0) { avformat_close_input(&fmt); return -3; }
    long long ms = 0;
    if (fmt->duration != AV_NOPTS_VALUE && fmt->duration > 0) ms = fmt->duration / 1000;
    const char *year = tag(fmt, "date");
    const char *track = tag(fmt, "track");
    int n = snprintf(out, (size_t)out_size, "%lld\x1f%s\x1f%s\x1f%s\x1f%s\x1f%s\x1f%s\x1f%s",
                     ms, tag(fmt, "title"), tag(fmt, "artist"), tag(fmt, "album"), track, year,
                     tag(fmt, "genre"), tag(fmt, "album_artist"));
    avformat_close_input(&fmt);
    if (n < 0 || n >= out_size) return -4;
    return n;
}
