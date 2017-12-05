#! /bin/bash

echo
echo Running before_install-osx.sh...
echo

echo Installing gcc/fortran
brew install gcc
brew link --overwrite gcc

brew install xz
brew install sqlite
brew install libiomp
brew install clang-omp
brew install ccache
export PATH=/usr/local/opt/ccache/libexec:$PATH
