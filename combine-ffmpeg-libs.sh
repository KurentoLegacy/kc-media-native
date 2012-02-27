#!/bin/sh

# Variables from build-ffmpeg.sh
EXTERNAL=$PWD/external
TARGET_DIR=$PWD/target
FFMPEG_INSTALL=$TARGET_DIR/ffmpeg_install
ARCHS="armv6 armv7 i386 "

BUILD_LIBS="libavcodec.a libavdevice.a libavfilter.a libavformat.a libavutil.a libswscale.a"

pushd `dirname $0`
pushd $FFMPEG_INSTALL

mkdir -p lib
mkdir -p include


for LIB in $BUILD_LIBS; do
  LIPO_CREATE=""
  for ARCH in $ARCHS; do
    LIPO_CREATE="$LIPO_CREATE-arch $ARCH ffmpeg-$ARCH/lib/$LIB "
  done
  OUTPUT="$FFMPEG_INSTALL/lib/$LIB"
  echo "Creating: $OUTPUT"
  echo "lipo -create $LIPO_CREATE -output $OUTPUT"
  lipo -create $LIPO_CREATE -output $OUTPUT
  lipo -info $OUTPUT

  echo " "
  echo
  echo
done

mkdir -p include
cp -R ffmpeg-armv6/include/* ./include

popd; popd

