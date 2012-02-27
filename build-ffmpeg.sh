#!/bin/sh

pushd `dirname $0`
pushd $EXTERNAL/ffmpeg

make clean && make distclean || echo clean OK

./configure \
        --prefix=$FFMPEG_INSTALL/ffmpeg-$DIST \
        --cc=${PLATFORM_BIN_DIR}/$CC \
        --as="gas-preprocessor.pl ${PLATFORM_BIN_DIR}/$CC " \
        --sysroot=$PLATFORM_SDK_DIR \
        --extra-cflags="$MY_CFLAGS " \
        --extra-ldflags="$MY_LDFLAGS " \
	$OPTS

make && make install

popd; popd

