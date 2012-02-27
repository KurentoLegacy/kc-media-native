#!/bin/sh

export EXTERNAL=$PWD/external
TARGET_DIR=$PWD/target
export FFMPEG_INSTALL=$TARGET_DIR/ffmpeg_install

GAS_PREPROCESSOR_DIR=$EXTERNAL/gas-preprocessor
PATH=$PATH:$GAS_PREPROCESSOR_DIR

rm -fr $FFMPEG_INSTALL
mkdir -p $FFMPEG_INSTALL

iphone_version=4.3
gccvers=4.2

if [ 0 -eq 0 ]; then
##################################################################
echo Build armv6
ARCHS="$ARCHS armv6 "
armarch=armv6
arch=arm
cpu=arm1176jzf-s
export OPTS="--enable-cross-compile --target-os=darwin --arch=$arch --cpu=$cpu "

platform=iPhoneOS

PLATFORM_DIR="/Developer/Platforms/${platform}.platform/Developer"
export PLATFORM_BIN_DIR="${PLATFORM_DIR}/usr/bin"
export PLATFORM_SDK_DIR="${PLATFORM_DIR}/SDKs/${platform}${iphone_version}.sdk"

export CC=gcc-4.2
export MY_CFLAGS="-arch ${armarch} "
export MY_LDFLAGS="-arch ${armarch} \
                -L${PLATFORM_SDK_DIR}/usr/lib/system "
export DIST=armv6

./build-ffmpeg.sh
##################################################################
fi
if [ 0 -eq 0 ]; then
##################################################################
echo build armv7
ARCHS="$ARCHS armv7 "
armarch=armv7
arch=arm
cpu=cortex-a8
export OPTS="--enable-cross-compile --target-os=darwin --arch=$arch --cpu=$cpu "

platform=iPhoneOS

PLATFORM_DIR="/Developer/Platforms/${platform}.platform/Developer"
export PLATFORM_BIN_DIR="${PLATFORM_DIR}/usr/bin"
export PLATFORM_SDK_DIR="${PLATFORM_DIR}/SDKs/${platform}${iphone_version}.sdk"

export CC=gcc-4.2
export MY_CFLAGS="-arch ${armarch} "
export MY_LDFLAGS="-arch ${armarch} \
                -L${PLATFORM_SDK_DIR}/usr/lib/system "
export DIST=armv7


./build-ffmpeg.sh
##################################################################
fi
if [ 0 -eq 0 ]; then
##################################################################
echo build i386
ARCHS="$ARCHS i386 "
export OPTS="--disable-yasm "

platform=iPhoneSimulator

PLATFORM_DIR="/Developer/Platforms/${platform}.platform/Developer"
export PLATFORM_BIN_DIR="${PLATFORM_DIR}/usr/bin"
PLATFORM_SDK_DIR="${PLATFORM_DIR}/SDKs/${platform}${iphone_version}.sdk"

export CC=i686-apple-darwin10-gcc-4.2.1
export MY_CFLAGS="-mdynamic-no-pic "
export MY_LDFLAGS="-L${PLATFORM_SDK_DIR}/usr/lib/system "
export DIST=i386

./build-ffmpeg.sh
##################################################################
fi

