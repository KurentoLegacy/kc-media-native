#!/bin/bash

echo "RUN configure-make-all.sh"

pushd `dirname $0`

#TEMPORAL
export MY_FFMPEG_INSTALL=$PWD/../target/ffmpeg_install

export MY_AMR_INSTALL=$PWD/../target/opencore-amr_install

export MY_X264_INSTALL=$PWD/../target/x264_install
#****************************





export armelf=armelf_linux_eabi.x;
export abi=arm-linux-androideabi;
export gccvers=4.4.3
export TOOLCHAIN_DIR=$PWD/toolchain

echo "NDK version: $(cat $ANDROID_NDK_HOME/RELEASE.TXT); ABI version: $abi-$gccvers";

#Create toolchain
#$ANDROID_NDK_HOME/build/tools/make-standalone-toolchain.sh \
#	--install-dir=$TOOLCHAIN_DIR \
#	--toolchain=$abi-$gccvers \
#	--ndk-dir=$ANDROID_NDK_HOME \
#	--system=linux-x86 \
#	--platform=android-8

export PATH=$PATH:$TOOLCHAIN_DIR:$TOOLCHAIN_DIR/bin

echo $PATH

export ARM_INC=$TOOLCHAIN_DIR/include
export ARM_LIB=$TOOLCHAIN_DIR/lib
export ARM_LIBO=$TOOLCHAIN_DIR/lib/gcc/$abi/$gccvers

#armarch="-D__ARM_ARCH_7A__ -march=armv7-a "
armarch="-D__ARM_ARCH_5TE__ -march=armv5te "

export MY_CFLAGS="-I$ARM_INC -DANDROID -fpic -mthumb-interwork -ffunction-sections \
		-funwind-tables -fstack-protector -fno-short-enums $armarch \
		-Wno-psabi -msoft-float -mthumb -Os -O -fomit-frame-pointer \
		-fno-strict-aliasing -finline-limit=64 -Wa,--noexecstack -MMD -MP "
export MY_LDFLAGS="-L$ARM_LIBO -nostdlib -Bdynamic  -Wl,--no-undefined -Wl,-z,noexecstack  \
		-Wl,-z,nocopyreloc -Wl,-soname,/system/lib/libz.so \
		-Wl,-rpath-link=$PLATFORM/usr/lib,-dynamic-linker=/system/bin/linker \
		-L$ARM_LIB  -lc -lm -ldl -Wl,--library-path=$PLATFORM/usr/lib/ \
		-Xlinker $TOOLCHAIN_DIR/sysroot/usr/lib/crtbegin_dynamic.o -Xlinker \
		$TOOLCHAIN_DIR/sysroot/usr/lib/crtend_android.o "






function die {
  echo "$1 failed" && exit -1
}

#  ./configure-make-x264.sh || die "configure-make-x264"

echo
echo
echo "+++++++++++++++++++++++++++++++++++++++++++"
echo "run configure-make-opencore-amr.sh"
export AMR_LIB_INC=$MY_AMR_INSTALL/include
export AMR_LIB_LIB=$MY_AMR_INSTALL/lib
./configure-make-opencore-amr.sh || die "configure-make-opencore-amr"
echo
echo "+++++++++++++++++++++++++++++++++++++++++++"
echo
echo
echo "+++++++++++++++++++++++++++++++++++++++++++"
echo "run configure-make-opencore-amr.sh"

#export USE_X264_TREE=x264-0.106.1741
#if [ "" == "$USE_X264_TREE" ]; then
#  echo "configure a LGPL ffmpeg, without H264 encoding support"
#  export X264_LIB_INC=;
#  export X264_LIB_LIB=;
#  export X264_C_EXTRA=;
#  export X264_LD_EXTRA=;
#  export X264_L=;
#  export X264_CONFIGURE_OPTS='--disable-gpl --disable-libx264';
#else
  echo "configure a GPL ffmpeg, with H264 encoding support at $USE_X264_TREE"
  export X264_LIB_INC=$MY_X264_INSTALL/include;
  export X264_LIB_LIB=$MY_X264_INSTALL/lib;
  export X264_C_EXTRA="-I$X264_LIB_INC ";
  export X264_LD_EXTRA="-L$X264_LIB_LIB -rpath-link=$X264_LIB_LIB ";
  export X264_L=-lx264;
  export X264_CONFIGURE_OPTS='--enable-gpl --enable-libx264 --enable-encoder=libx264';
#fi

./configure-make-x264.sh || die "configure-make-opencore-amr"
echo
echo "+++++++++++++++++++++++++++++++++++++++++++"
echo
echo
echo
echo
echo "+++++++++++++++++++++++++++++++++++++++++++"
echo "run configure-make-ffmpeg.sh"
./configure-make-ffmpeg.sh || die "configure-make-ffmpeg"
echo
echo "+++++++++++++++++++++++++++++++++++++++++++"
echo
echo


popd

