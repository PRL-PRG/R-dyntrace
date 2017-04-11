#!/bin/bash

export CCACHE_CPP2=yes
export CC="ccache clang"
export CXX="ccache clang++ -std=c++14 -stdlib=libc++"
sh configure --with-blas --with-lapack --without-ICU --without-x --without-tcltk --without-aqua --without-recommended-packages --without-internal-tzcode --with-included-gettext --enable-rdt
make -j5
