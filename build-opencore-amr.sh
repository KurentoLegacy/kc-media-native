#!/bin/bash

pushd `dirname $0`
pushd $EXTERNAL/opencore-amr

if [ "" == "$AMR_INSTALL" ]; then
  echo "Please set AMR_INSTALL to the location where amr should be installed.";
  exit -1;
fi

aclocal && \
autoheader && \
libtoolize --automake --copy --force && \
automake --add-missing --copy && \
autoconf && \
autoreconf -i

make clean && make distclean || echo "clean OK"

export LD="${PLATFORM_BIN_DIR}/$MY_LD"
export CC="${PLATFORM_BIN_DIR}/$MY_CC $MY_CFLAGS "
export CXX="${PLATFORM_BIN_DIR}/$MY_CXX $MY_CFLAGS "
#export CFLAGS="$MY_CFLAGS"
#export CXXFLAGS="$MY_CFLAGS"
export LDFLAGS="-Wl,-syslibroot,$PLATFORM_SDK_DIR "

./configure --prefix=$AMR_INSTALL/opencore-amr-$DIST \
	--disable-shared \
	$OPTS

make && make install

popd; popd
