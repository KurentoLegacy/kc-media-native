#!/bin/sh

BUILD_LIBS="libavcodec.a libavdevice.a libavfilter.a libavformat.a \
	libavutil.a libswscale.a"

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
  echo
done

arch=`echo $ARCHS | cut -d" " -f1`
if [ -n "$arch" ]; then
  cp -R ffmpeg-$arch/include/* ./include
fi

popd; popd
