#!/bin/bash

echo "RUN configure-make-all.sh"

pushd `dirname $0`

#TEMPORAL
export MY_FFMPEG_INSTALL=$PWD/../target/ffmpeg_install
export MY_AMR_INSTALL=$PWD/../target/opencore-amr_install
#****************************

export AMR_LIB_INC=$MY_AMR_INSTALL/include
export AMR_LIB_LIB=$MY_AMR_INSTALL/lib




export armelf=armelf_linux_eabi.x;
export abi=arm-linux-androideabi;
export gccvers=4.4.3
export TOOLCHAIN_DIR=$PWD/toolchain

echo "NDK version: $(cat $ANDROID_NDK_HOME/RELEASE.TXT); ABI version: $abi-$gccvers";

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


export MY_CFLAGS="-I$ARM_INC -DANDROID -fpic -mthumb-interwork -ffunction-sections \
		-funwind-tables -fstack-protector -fno-short-enums -D__ARM_ARCH_7A__ \
		-Wno-psabi -march=armv7-a -msoft-float -mthumb -Os -O -fomit-frame-pointer \
		-fno-strict-aliasing -finline-limit=64 -Wa,--noexecstack -MMD -MP "
export MY_LDFLAGS="-L$ARM_LIBO -nostdlib -Bdynamic  -Wl,--no-undefined -Wl,-z,noexecstack  \
		-Wl,-z,nocopyreloc -Wl,-soname,/system/lib/libz.so \
		-Wl,-rpath-link=$PLATFORM/usr/lib,-dynamic-linker=/system/bin/linker \
		-L$ARM_LIB  -lc -lm -ldl -Wl,--library-path=$PLATFORM/usr/lib/ \
		-Xlinker $TOOLCHAIN_DIR/sysroot/usr/lib/crtbegin_dynamic.o -Xlinker $TOOLCHAIN_DIR/sysroot/usr/lib/crtend_android.o "






function die {
  echo "$1 failed" && exit -1
}

#  ./configure-make-x264.sh || die "configure-make-x264"

echo
echo
echo "+++++++++++++++++++++++++++++++++++++++++++"
echo "run configure-make-opencore-amr.sh"
#./configure-make-opencore-amr.sh || die "configure-make-opencore-amr"
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

