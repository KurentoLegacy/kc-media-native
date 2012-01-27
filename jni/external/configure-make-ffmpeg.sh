#!/bin/bash

pushd `dirname $0`
pushd ffmpeg

#make clean && make distclean || echo OK

./configure --prefix=${MY_FFMPEG_INSTALL} \
	--target-os=linux \
	--arch=arm \
	--enable-cross-compile \
	--cc=$abi-gcc \
	--cross-prefix=$abi- \
	--nm=$abi-nm \
	--enable-static \
	--disable-shared \
	--enable-armv5te --enable-armv6 --enable-armv6t2 --enable-armvfp \
	--disable-asm --disable-yasm --enable-neon --enable-pic \
	--disable-amd3dnow --disable-amd3dnowext --disable-mmx --disable-mmx2 --disable-sse --disable-ssse3 \
	--enable-version3 \
	--disable-nonfree \
	--disable-stripping \
	--disable-doc \
	--disable-ffplay \
	--disable-ffmpeg \
	--disable-ffprobe \
	--disable-ffserver \
	--disable-avdevice \
	--disable-avfilter \
	--disable-devices \
	--disable-encoders \
	--enable-encoder=h263p --enable-encoder=mpeg4 \
	--enable-encoder=mp2 --enable-encoder=aac --enable-encoder=pcm_mulaw --enable-encoder=pcm_alaw \
	--extra-cflags="$MY_CFLAGS " \
	--extra-ldflags="$MY_LDFLAGS -Wl,-T,$TOOLCHAIN_DIR/arm-linux-androideabi/lib/ldscripts/$armelf \
			$TOOLCHAIN_DIR/lib/gcc/$abi/$gccvers/crtbegin.o $ARM_LIBO/crtend.o " \
	--extra-libs="-lgcc " \
	--enable-encoder=libopencore_amrnb \
	--enable-libopencore-amrnb \
	--extra-libs="-lopencore-amrnb " \
	--extra-cflags="-I$AMR_LIB_INC " \
	--extra-ldflags="-L$AMR_LIB_LIB "


make
make install

popd; popd

