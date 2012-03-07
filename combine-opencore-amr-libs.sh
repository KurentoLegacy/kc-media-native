#!/bin/sh
BUILD_LIBS="libopencore-amrnb.a libopencore-amrwb.a"

pushd `dirname $0`
pushd $AMR_INSTALL

mkdir -p lib
mkdir -p include

for LIB in $BUILD_LIBS; do
  LIPO_CREATE=""
  for ARCH in $ARCHS; do
    LIPO_CREATE="$LIPO_CREATE -arch $ARCH opencore-amr-$ARCH/lib/$LIB "
  done
  OUTPUT="$AMR_INSTALL/lib/$LIB"
  echo "Creating: $OUTPUT"
  echo "lipo -create $LIPO_CREATE -output $OUTPUT"
  lipo -create $LIPO_CREATE -output $OUTPUT
  lipo -info $OUTPUT
  echo
done

arch=`echo $ARCHS | cut -d" " -f1`
if [ -n "$arch" ]; then
  cp -R opencore-amr-$arch/include/* ./include
fi

popd; popd
