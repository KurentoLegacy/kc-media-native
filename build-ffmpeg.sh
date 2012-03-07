#!/bin/sh

pushd `dirname $0`
pushd $EXTERNAL/ffmpeg

make clean && make distclean || echo clean OK

./configure \
        --prefix=$FFMPEG_INSTALL/ffmpeg-$DIST \
        --cc=${PLATFORM_BIN_DIR}/$CC \
        --as="$CPP ${PLATFORM_BIN_DIR}/$CC " \
        --sysroot=$PLATFORM_SDK_DIR \
        --extra-cflags="$MY_CFLAGS $AMR_C_EXTRA $X264_C_EXTRA " \
        --extra-ldflags="$MY_LDFLAGS $AMR_LD_EXTRA $X264_LD_EXTRA " \
	--extra-libs="$AMR_L $X264_L " \
	$OPTS \
	$AMR_CONFIGURE_OPTS $X264_CONFIGURE_OPTS

make && make install

popd; popd
