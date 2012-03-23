#!/bin/sh

export EXTERNAL=$PWD/external
TARGET_DIR=$PWD/target
export FFMPEG_INSTALL=$TARGET_DIR/ffmpeg_install
export AMR_INSTALL=$TARGET_DIR/opencore-amr_install
export X264_INSTALL=$TARGET_DIR/x264_install


iphone_version=4.3
gccvers=4.2


function space {
  for i in `seq 1 3`; do echo ""; done
}

if false; then
### opencore-amr ###############################################################
echo "### build opencore-amr ##################################################"
rm -fr $AMR_INSTALL
mkdir -p $AMR_INSTALL

if [ 0 -eq 0 ]; then
##################################################################
arch=armv6
echo "+++ build opencore-amr for $arch +++"
export ARCHS="$arch "
platform=iPhoneOS

PLATFORM_DIR="/Developer/Platforms/${platform}.platform/Developer"
export PLATFORM_BIN_DIR="${PLATFORM_DIR}/usr/bin"
export PLATFORM_SDK_DIR="${PLATFORM_DIR}/SDKs/${platform}${iphone_version}.sdk"

export MY_CFLAGS="-arch $arch --sysroot=$PLATFORM_SDK_DIR "
export DIST=$arch

export MY_LD="ld"
export MY_CC="gcc"
export MY_CXX="g++"

host="arm-apple-darwin"
target="darwin"
export OPTS="--host=$host --target=$target"

./build-opencore-amr.sh
echo "+++ build opencore-amr for $arch OK +++"
##################################################################
fi
space
if [ 0 -eq 0 ]; then
##################################################################
arch=armv7
echo "+++ build opencore-amr for $arch +++"
export ARCHS="$ARCHS $arch "
platform=iPhoneOS

PLATFORM_DIR="/Developer/Platforms/${platform}.platform/Developer"
export PLATFORM_BIN_DIR="${PLATFORM_DIR}/usr/bin"
export PLATFORM_SDK_DIR="${PLATFORM_DIR}/SDKs/${platform}${iphone_version}.sdk"

export MY_CFLAGS="-arch $arch --sysroot=$PLATFORM_SDK_DIR "
export DIST=$arch

export MY_LD="ld"
export MY_CC="gcc"
export MY_CXX="g++"

host="arm-apple-darwin"
target="darwin"
export OPTS="--host=$host --target=$target"

./build-opencore-amr.sh
echo "+++ build opencore-amr for $arch OK +++"
##################################################################
fi
space
if [ 0 -eq 0 ]; then
##################################################################
arch=i386
echo "+++ build opencore-amr for $arch +++"
export ARCHS="$ARCHS $arch "
platform=iPhoneSimulator

PLATFORM_DIR="/Developer/Platforms/${platform}.platform/Developer"
export PLATFORM_BIN_DIR="${PLATFORM_DIR}/usr/bin"
export PLATFORM_SDK_DIR="${PLATFORM_DIR}/SDKs/${platform}${iphone_version}.sdk"

export MY_CFLAGS="-arch $arch --sysroot=$PLATFORM_SDK_DIR "
export DIST=$arch

export MY_LD="ld"
export MY_CC="gcc"
export MY_CXX="g++"

host="i686-apple-darwin"
target="darwin"
export OPTS="--host=$host --target=$target"

./build-opencore-amr.sh
echo "+++ build opencore-amr for $arch OK +++"
##################################################################
fi
fi

echo "+++ combine opencore-amr libs +++"
./combine-opencore-amr-libs.sh
echo "+++ combine opencore-amr libs OK +++"

export AMR_C_EXTRA="-I$AMR_INSTALL/include "
export AMR_LD_EXTRA="-L$AMR_INSTALL/lib "
export AMR_L="-lopencore-amrnb"
export AMR_CONFIGURE_OPTS="--enable-version3 \
	--enable-libopencore-amrnb --enable-encoder=libopencore_amrnb";

echo "### build opencore-amr OK ###############################################"
### opencore-amr ###############################################################



space



if false; then
### x264 #######################################################################
echo "### build x264 ##########################################################"
rm -fr $X264_INSTALL
mkdir -p $X264_INSTALL

if [ 0 -eq 0 ]; then
##################################################################
arch=armv6
echo "+++ build x264 for $arch +++"
export ARCHS="$arch "
platform=iPhoneOS

PLATFORM_DIR="/Developer/Platforms/${platform}.platform/Developer"
export PLATFORM_BIN_DIR="${PLATFORM_DIR}/usr/bin"
export PLATFORM_SDK_DIR="${PLATFORM_DIR}/SDKs/${platform}${iphone_version}.sdk"

export DIST=$arch
export MY_CC="gcc"
host="arm-apple-darwin"
export MY_CFLAGS="-arch $arch"
export MY_LDFLAGS="-arch $arch -L${PLATFORM_SDK_DIR}/usr/lib/system"
export OPTS="--host=$host --enable-pic --disable-asm "

./build-x264.sh
echo "+++ build x264 for $arch OK +++"
##################################################################
fi
space
if [ 0 -eq 0 ]; then
##################################################################
arch=armv7
echo "+++ build x264 for $arch +++"
export ARCHS="$ARCHS $arch "
platform=iPhoneOS

PLATFORM_DIR="/Developer/Platforms/${platform}.platform/Developer"
export PLATFORM_BIN_DIR="${PLATFORM_DIR}/usr/bin"
export PLATFORM_SDK_DIR="${PLATFORM_DIR}/SDKs/${platform}${iphone_version}.sdk"

export DIST=$arch
export MY_CC="gcc"
host="arm-apple-darwin"
export MY_CFLAGS="-arch $arch"
export MY_LDFLAGS="-arch $arch -L${PLATFORM_SDK_DIR}/usr/lib/system"
export OPTS="--host=$host --enable-pic "

./build-x264.sh
echo "+++ build x264 for $arch OK +++"
##################################################################
fi
space
if [ 0 -eq 0 ]; then
##################################################################
arch=i386
echo "+++ build x264 for $arch +++"
export ARCHS="$ARCHS $arch "
platform=iPhoneSimulator

PLATFORM_DIR="/Developer/Platforms/${platform}.platform/Developer"
export PLATFORM_BIN_DIR="${PLATFORM_DIR}/usr/bin"
export PLATFORM_SDK_DIR="${PLATFORM_DIR}/SDKs/${platform}${iphone_version}.sdk"

export DIST=$arch
export MY_CC="i686-apple-darwin10-gcc-4.2.1"
export MY_CFLAGS=""
export MY_LDFLAGS="-Wl,-syslibroot,$PLATFORM_SDK_DIR "
export OPTS="--disable-asm"

./build-x264.sh
echo "+++ build x264 for $arch OK +++"
##################################################################
fi
fi

echo "+++ combine x264 libs +++"
./combine-x264-libs.sh
echo "+++ combine x264 libs OK +++"

export X264_C_EXTRA="-I$X264_INSTALL/include "
export X264_LD_EXTRA="-L$X264_INSTALL/lib "
export X264_L="-lx264"
export X264_CONFIGURE_OPTS="--enable-gpl --enable-libx264 --enable-encoder=libx264";

echo "### build x264 OK #######################################################"
### opencore-amr ###############################################################



space



if false; then
### ffmpeg #####################################################################
echo "### build ffmpeg ########################################################"
rm -fr $FFMPEG_INSTALL
mkdir -p $FFMPEG_INSTALL

GAS_PREPROCESSOR_DIR=$EXTERNAL/gas-preprocessor
PATH=$PATH:$GAS_PREPROCESSOR_DIR
export CPP="gas-preprocessor.pl"

if [ 0 -eq 0 ]; then
##################################################################
echo Build armv6
export ARCHS="armv6 "
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
export ARCHS="$ARCHS armv7 "
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
export ARCHS="$ARCHS i386 "
export OPTS="--disable-yasm "

platform=iPhoneSimulator

PLATFORM_DIR="/Developer/Platforms/${platform}.platform/Developer"
export PLATFORM_BIN_DIR="${PLATFORM_DIR}/usr/bin"
export PLATFORM_SDK_DIR="${PLATFORM_DIR}/SDKs/${platform}${iphone_version}.sdk"

export CC=i686-apple-darwin10-gcc-4.2.1
export MY_CFLAGS="-mdynamic-no-pic "
export MY_LDFLAGS="-L${PLATFORM_SDK_DIR}/usr/lib/system "
export DIST=i386

./build-ffmpeg.sh
##################################################################
fi
fi
export ARCHS="armv6 armv7 i386 "
echo "+++ combine ffmpeg libs +++"
./combine-ffmpeg-libs.sh
echo "+++ combine ffmpeg libs OK +++"
echo "### build ffmpeg OK #####################################################"
### ffmpeg #####################################################################
