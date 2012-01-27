#!/bin/bash

pushd `dirname $0`
pushd x264

#make distclean

if [ "" == "$MY_X264_INSTALL" ]; then
  echo "Please set MY_X264_INSTALL to the location where x264 should be installed.";
  exit -1;
fi

./configure --prefix=$MY_X264_INSTALL \
	--disable-gpac \
	--extra-cflags="$MY_CFLAGS " \
	--extra-ldflags="$MY_LDFLAGS -lgcc " \
	--disable-asm \
	--cross-prefix=$abi- \
	--disable-pthread \
	--host=arm-linux

make
make install-lib-static

popd; popd

