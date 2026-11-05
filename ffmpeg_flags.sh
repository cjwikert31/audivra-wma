# The ONLY parts of FFmpeg we build: the ASF file reader and the WMA decoders.
# Everything else is switched off, which keeps the library tiny and the license LGPL.
FFMPEG_FLAGS=(
  --disable-everything --disable-programs --disable-doc --disable-debug
  --disable-avdevice --disable-avfilter --disable-swscale --disable-network --disable-autodetect
  --disable-gpl --disable-nonfree --disable-version3
  --enable-small --enable-pic --disable-shared --enable-static
  --enable-protocol=file
  --enable-demuxer=asf
  --enable-decoder=wmav1,wmav2,wmapro,wmalossless,wmavoice
  --enable-swresample
)
