#!/bin/bash
set -x
API=28
NDK=/home/mlgc4869/android-ndk-r20
PREFIX=$(pwd)/android/out/armeabi-v7a
PLATFORM=arm-linux-androideabi
TOOLCHAIN=$NDK/toolchains/llvm/prebuilt/linux-x86_64
SYSROOT=$TOOLCHAIN/sysroot
ASM=$SYSROOT/usr/include/$PLATFORM
CROSS_PREFIX=$TOOLCHAIN/bin/$PLATFORM-
ANDROID_CROSS_PREFIX=$TOOLCHAIN/bin/armv7a-linux-androideabi28-
function function_one
{
  ./configure \
    --prefix=$PREFIX \
    --enable-shared \
    --disable-static \
    --disable-doc \
    --enable-ffmpeg \
    --enable-ffplay \
    --enable-ffprobe \
    --enable-avdevice \
    --cross-prefix=$CROSS_PREFIX \
    --cc=${ANDROID_CROSS_PREFIX}clang \
    --target-os=android \
    --arch=armv7-a \
    --enable-cross-compile \
    --sysroot=$SYSROOT \
    --disable-x86asm \
    --extra-cflags="-I$ASM -I/usr/include/SDL2 -isysroot $SYSROOT -Os -fpic"
}
CPU=armv7-a
PREFIX=$(pwd)/android/$CPU
function_one
