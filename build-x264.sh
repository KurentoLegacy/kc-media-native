#!/bin/bash

pushd `dirname $0`
pushd $EXTERNAL/x264

if [ "" == "$X264_INSTALL" ]; then
  echo "Please set X264_INSTALL to the location where x264 should be installed.";
  exit -1;
fi

make distclean || echo clean OK

mkdir -p $X264_INSTALL/x264-$DIST

export CC="${PLATFORM_BIN_DIR}/$MY_CC"
./configure \
	--prefix=$X264_INSTALL/x264-$DIST \
	--sysroot=$PLATFORM_SDK_DIR \
	--extra-cflags="$MY_CFLAGS" \
	--extra-ldflags="$MY_LDFLAGS" \
	$OPTS

make && make install-lib-static

popd; popd
