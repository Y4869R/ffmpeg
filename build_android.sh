#!/bin/bash
set -x
API=28
NDK=/home/SERAPHIC/chendh/ffmpeg/ndk/android-ndk-r17c
SYSROOT=$NDK/platforms/android-$API/arch-arm/
TOOLCHAIN=$NDK/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64
function function_one
{

  ./configure \
    --prefix=$PREFIX \
    --disable-shared \
    --enable-static \
    --disable-doc \
    --disable-ffplay \
    --disable-ffprobe \
    --disable-symver \
    --disable-ffmpeg \
    --cc=$TOOLCHAIN/bin/arm-linux-androideabi-gcc \
    --cross-prefix=$TOOLCHAIN/bin/arm-linux-androideabi- \
    --target-os=linux \
    --arch=arm \
    --enable-cross-compile \
    --sysroot=$SYSROOT \
    --extra-cflags="-I$NDK/sysroot/usr/include/arm-linux-androideabi -isysroot $NDK/sysroot -fPIC -DANDROID -D__thumb__ -mthumb -Wfatal-errors -Wno-deprecated -mfloat-abi=softfp -marm -march=armv7-a" \
    --enable-neon

  make clean all
  make
  make install
}
CPU=armv7-a
PREFIX=$(pwd)/android/$CPU
function_one
