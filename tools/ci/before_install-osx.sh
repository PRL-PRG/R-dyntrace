#! /bin/bash

echo
echo Running before_install-osx.sh...
echo

echo Installing gcc/fortran
brew install gcc

brew install xz
brew install sqlite

brew install ccache
export PATH=$PATH:/usr/local/opt/ccache/libexec
