#!/usr/bin/env bash
# Builds libaudivrawma.so (tiny FFmpeg with only WMA support) for one Android CPU type.
# Usage: ./build_android.sh arm64-v8a   (also: armeabi-v7a, x86_64)
# Needs the Android NDK; GitHub's build machines already have it (ANDROID_NDK_LATEST_HOME).
set -euo pipefail
ABI="$1"
NDK="${ANDROID_NDK_ROOT:-${ANDROID_NDK_LATEST_HOME:?Android NDK not found}}"
API=24
HERE="$(cd "$(dirname "$0")" && pwd)"
TOOLCHAIN="$NDK/toolchains/llvm/prebuilt/linux-x86_64"

case "$ABI" in
  arm64-v8a)   ARCH=aarch64; TRIPLE=aarch64-linux-android ;;
  armeabi-v7a) ARCH=arm;     TRIPLE=armv7a-linux-androideabi ;;
  x86_64)      ARCH=x86_64;  TRIPLE=x86_64-linux-android ;;
  *) echo "Unknown ABI $ABI"; exit 1 ;;
esac
CC="$TOOLCHAIN/bin/${TRIPLE}${API}-clang"

FFSRC="$HERE/ffmpeg-src"
if [ ! -d "$FFSRC" ]; then
  git clone --depth 1 --branch n7.1 https://github.com/FFmpeg/FFmpeg.git "$FFSRC"
fi

# shellcheck source=ffmpeg_flags.sh
source "$HERE/ffmpeg_flags.sh"

BUILD="$HERE/build/$ABI"
rm -rf "$BUILD"; mkdir -p "$BUILD"; cd "$BUILD"
"$FFSRC/configure" --prefix="$BUILD/install" "${FFMPEG_FLAGS[@]}" \
  --target-os=android --arch="$ARCH" --enable-cross-compile \
  --cc="$CC" --ar="$TOOLCHAIN/bin/llvm-ar" --nm="$TOOLCHAIN/bin/llvm-nm" \
  --ranlib="$TOOLCHAIN/bin/llvm-ranlib" --strip="$TOOLCHAIN/bin/llvm-strip" \
  --disable-asm \
  --extra-cflags="-O2 -fPIC" \
  --extra-ldflags="-Wl,-z,max-page-size=16384"
make -j"$(nproc)"
make install

OUT="$HERE/out/jniLibs/$ABI"
mkdir -p "$OUT"
"$CC" -shared -fPIC -O2 -I"$BUILD/install/include" \
  "$HERE/wma_core.c" "$HERE/wma_jni.c" \
  "$BUILD/install/lib/libavformat.a" "$BUILD/install/lib/libavcodec.a" \
  "$BUILD/install/lib/libswresample.a" "$BUILD/install/lib/libavutil.a" \
  -lm \
  -Wl,-z,max-page-size=16384 -Wl,-z,noexecstack -Wl,--gc-sections -Wl,--exclude-libs,ALL \
  -o "$OUT/libaudivrawma.so"
"$TOOLCHAIN/bin/llvm-strip" --strip-unneeded "$OUT/libaudivrawma.so"
ls -la "$OUT/libaudivrawma.so"
