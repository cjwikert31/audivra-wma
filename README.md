# Audivra WMA library

A tiny audio library that lets Audivra play Windows Media Audio (.wma) files.

It is built from [FFmpeg](https://ffmpeg.org) (version 7.1) with **only** the ASF file reader and the
WMA decoders (WMA 1, WMA 2, WMA Pro, WMA Lossless, WMA Voice) switched on. Everything else is off.

## License

FFmpeg is licensed under the **GNU Lesser General Public License, version 2.1 or later (LGPL-2.1+)**.
This build is configured without GPL or "non-free" parts (see `ffmpeg_flags.sh`).
The FFmpeg source code is at https://github.com/FFmpeg/FFmpeg (tag `n7.1`), and the exact recipe used to
build it is in this repository (`build_android.sh`, `ffmpeg_flags.sh`), so anyone can rebuild the library.
The small bridge code in `wma_core.c` and `wma_jni.c` is Audivra's own.

FFmpeg is a trademark of Fabrice Bellard, originator of the FFmpeg project.

## How it is built

Open the **Actions** tab on GitHub, choose **Build WMA library**, press **Run workflow**, and download
the `audivra-wma-libs` file when it finishes.
