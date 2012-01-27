#!/bin/bash

pushd `dirname $0`
pushd ffmpeg

make clean
make distclean


export ARM_INC=$TOOLCHAIN_DIR/include
export ARM_LIB=$TOOLCHAIN_DIR/lib
export ARM_LIBO=$TOOLCHAIN_DIR/lib/gcc/$abi/$gccvers

echo
echo "ARM_LIBO: $ARM_LIBO"
echo
echo






export MY_CFLAGS="-I$ARM_INC -DANDROID -fpic -mthumb-interwork -ffunction-sections \
		-funwind-tables -fstack-protector -fno-short-enums -D__ARM_ARCH_7A__ \
		-Wno-psabi -march=armv7-a -msoft-float -mthumb -Os -O -fomit-frame-pointer \
		-fno-strict-aliasing -finline-limit=64 -Wa,--noexecstack -MMD -MP "
export MY_LDFLAGS="-L$ARM_LIBO -nostdlib -Bdynamic  -Wl,--no-undefined -Wl,-z,noexecstack  \
		-Wl,-z,nocopyreloc -Wl,-soname,/system/lib/libz.so \
		-Wl,-rpath-link=$PLATFORM/usr/lib,-dynamic-linker=/system/bin/linker \
		-L$ARM_LIB  -lc -lm -ldl -Wl,--library-path=$PLATFORM/usr/lib/ \
		-Xlinker $TOOLCHAIN_DIR/sysroot/usr/lib/crtbegin_dynamic.o -Xlinker $TOOLCHAIN_DIR/sysroot/usr/lib/crtend_android.o "

./configure --target-os=linux \
	--arch=arm \
	--prefix=${MY_FFMPEG_INSTALL} \
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
	--enable-encoder=libopencore_amrnb --enable-encoder=mp2 --enable-encoder=aac --enable-encoder=pcm_mulaw --enable-encoder=pcm_alaw \
	--extra-cflags="$MY_CFLAGS" \
	--extra-ldflags="$MY_LDFLAGS -Wl,-T,$TOOLCHAIN_DIR/arm-linux-androideabi/lib/ldscripts/$armelf \
			$TOOLCHAIN_DIR/lib/gcc/$abi/$gccvers/crtbegin.o $ARM_LIBO/crtend.o " \
	--extra-libs="-lgcc " \


make

popd; popd

