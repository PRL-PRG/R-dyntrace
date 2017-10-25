#!/bin/bash

export R_COMPILE_PKGS=1
export R_DISABLE_BYTECODE=0
export R_ENABLE_JIT=0
export R_KEEP_PKG_SOURCE=yes

bin/R --slave --no-restore --file=install-r-package-anthology.R | tee "install-r-package-anthology.log" 

