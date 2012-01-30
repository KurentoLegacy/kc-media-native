#!/bin/bash

echo "run configure-make-all.sh"

# ensure ANDROID_NDK_HOME is set
if [ "" == "$ANDROID_NDK_HOME" ]; then
  echo "Please set ANDROID_NDK_HOME to your Android Native Development Kit path.";
  exit -1;
fi

pushd `dirname $0`

export EXTERNAL=$PWD/external
TARGET_DIR=$PWD/target
MARK_FILE=$TARGET_DIR/config.mark

export armelf=armelf_linux_eabi.x;
export abi=arm-linux-androideabi;
export gccvers=4.4.3
export TOOLCHAIN_DIR=$TARGET_DIR/toolchain

export PATH=$PATH:$TOOLCHAIN_DIR:$TOOLCHAIN_DIR/bin

export ARM_INC=$TOOLCHAIN_DIR/include
export ARM_LIB=$TOOLCHAIN_DIR/lib
export ARM_LIBO=$TOOLCHAIN_DIR/lib/gcc/$abi/$gccvers

armarch="-D__ARM_ARCH_7A__ -march=armv7-a "
#armarch="-D__ARM_ARCH_5TE__ -march=armv5te "

export MY_CFLAGS="-I$ARM_INC -DANDROID -fpic -mthumb-interwork -ffunction-sections \
		-funwind-tables -fstack-protector -fno-short-enums $armarch \
		-Wno-psabi -msoft-float -mthumb -Os -O -fomit-frame-pointer \
		-fno-strict-aliasing -finline-limit=64 -Wa,--noexecstack -MMD -MP "
export MY_LDFLAGS="-L$ARM_LIBO -nostdlib -Bdynamic  -Wl,--no-undefined -Wl,-z,noexecstack \
		-Wl,-z,nocopyreloc -Wl,-soname,/system/lib/libz.so \
		-Wl,-rpath-link=$PLATFORM/usr/lib,-dynamic-linker=/system/bin/linker \
		-L$ARM_LIB  -lc -lm -ldl -Wl,--library-path=$PLATFORM/usr/lib/ \
		-Xlinker $TOOLCHAIN_DIR/sysroot/usr/lib/crtbegin_dynamic.o -Xlinker \
		$TOOLCHAIN_DIR/sysroot/usr/lib/crtend_android.o "

if [ "" == "$ENABLE_X264" ]; then
  ENABLE_X264="NO"
else
  ENABLE_X264="YES"
fi

#Check if must run
if [ -f $MARK_FILE ]
then
  OLD_ANDROID_NDK_HOME=`grep ANDROID_NDK_HOME $MARK_FILE | sed -e "s/ANDROID\_NDK\_HOME=//"`
  OLD_ENABLE_X264=`grep ENABLE_X264 $MARK_FILE | sed -e "s/ENABLE\_X264\=//"`
  echo "OLD_ANDROID_NDK_HOME=$OLD_ANDROID_NDK_HOME"
  echo "OLD_ENABLE_X264=$OLD_ENABLE_X264"
else
  OLD_ANDROID_NDK_HOME=""
  OLD_ENABLE_X264=""
fi

if [[ "$OLD_ANDROID_NDK_HOME" = "$ANDROID_NDK_HOME" && "$OLD_ENABLE_X264" = "$ENABLE_X264" ]]
then
  echo "No need to run config again, exiting..."
  exit 0
fi


echo "ANDROID_NDK_HOME=$ANDROID_NDK_HOME" > $MARK_FILE
echo "ENABLE_X264=$ENABLE_X264" >> $MARK_FILE
echo "NDK version: $(cat $ANDROID_NDK_HOME/RELEASE.TXT); ABI version: $abi-$gccvers";

mkdir -p $TARGET_DIR
#Create toolchain
$ANDROID_NDK_HOME/build/tools/make-standalone-toolchain.sh \
	--install-dir=$TOOLCHAIN_DIR \
	--toolchain=$abi-$gccvers \
	--ndk-dir=$ANDROID_NDK_HOME \
	--system=linux-x86 \
	--platform=android-8

function die {
  echo "$1 failed" && exit -1
}

echo "+++++++++++++++++++++++++++++++++++++++++++"
echo "run configure-make-opencore-amr.sh"
AMR_LIB_INC=$MY_AMR_INSTALL/include;
AMR_LIB_LIB=$MY_AMR_INSTALL/lib;
export AMR_C_EXTRA="-I$AMR_LIB_INC "
export AMR_LD_EXTRA="-L$AMR_LIB_LIB "
export AMR_L="-lopencore-amrnb"
export AMR_CONFIGURE_OPTS="--enable-libopencore-amrnb --enable-encoder=libopencore_amrnb";
./configure-make-opencore-amr.sh || die "configure-make-opencore-amr"
echo "+++++++++++++++++++++++++++++++++++++++++++"

echo
echo

echo "+++++++++++++++++++++++++++++++++++++++++++"
echo "run configure-make-x264.sh"
if [ "NO" == "$ENABLE_X264" ]; then
  echo "configure a LGPL ffmpeg, without H264 encoding support"
  export X264_C_EXTRA=;
  export X264_LD_EXTRA=;
  export X264_L=;
  export X264_CONFIGURE_OPTS='--disable-gpl --disable-libx264';
else
  echo "configure a GPL ffmpeg, with H264 encoding support"
  X264_LIB_INC=$MY_X264_INSTALL/include;
  X264_LIB_LIB=$MY_X264_INSTALL/lib;
  export X264_C_EXTRA="-I$X264_LIB_INC ";
  export X264_LD_EXTRA="-L$X264_LIB_LIB -rpath-link=$X264_LIB_LIB ";
  export X264_L="-lx264";
  export X264_CONFIGURE_OPTS='--enable-gpl --enable-libx264 --enable-encoder=libx264';
  ./configure-make-x264.sh || die "configure-make-x264"
fi
echo "+++++++++++++++++++++++++++++++++++++++++++"

echo
echo

echo "+++++++++++++++++++++++++++++++++++++++++++"
echo "run configure-make-ffmpeg.sh"
./configure-make-ffmpeg.sh || die "configure-make-ffmpeg"
echo "+++++++++++++++++++++++++++++++++++++++++++"

popd

