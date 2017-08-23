#!/bin/bash

#CMD='bin/Rscript rdt-plugins/promises/R/benchmark.R'
#CMD='bin/Rscript compose_testable_vignettes.R'
#CMD='bin/R --slave --no-restore --debugger=gdb --file=compose_testable_vignettes.R --args'
CMD='bin/R --slave --no-restore --file=sanity-check.R --args'

export R_COMPILE_PKGS=1
export R_DISABLE_BYTECODE=0
export R_ENABLE_JIT=0
export R_KEEP_PKG_SOURCE=yes

COMPILE_VIGNETTE=false

PACKAGES=

if $COMPILE_VIGNETTE
then 
    CMD="$CMD --compile"        
fi    


if [ $# -ge 1 ]
then
    PACKAGES="$@"
else
    #PACKAGES="vcd rpart survival mclust party mvtnorm igraph"
    #ALL_PACKAGES="grid ggplot2 haven readr readxl stringr tibble tidyverse digest colorspace kernlab vcd rpart survival mclust party mvtnorm igraph dplyr"
    PACKAGES=`cat r-package-anthology.csv | grep -v '^#' | grep -v '^$' | tr -s ' ' | tail -n +2 | cut -f 2 -d';' | xargs echo`
fi

echo > packages_done

for i in $PACKAGES
do 
    echo "$CMD $i"
    time $CMD $i 2>&1 | tee "$i.log" 
    echo "$i" >> packages_done
done   

