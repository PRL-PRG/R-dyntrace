#!/bin/bash

CMD='bin/Rscript rdt-plugins/promises/R/benchmark.R'

PACKAGES=

if [ $# -ge 1 ]
then
    PACKAGES="$@"
else 
    PACKAGES="vcd rpart survival mclust party mvtnorm igraph"
    ALL_PACKAGES="grid ggplot2 haven readr readxl stringr tibble tidyverse digest colorspace kernlab vcd rpart survival mclust party mvtnorm igraph dplyr"
fi

echo > packages_done

for i in $PACKAGES
do 
    echo "$CMD $i"
    time $CMD $i 2>&1 | tee "$i.log" 
    echo "$i" >> packages_done
done   

