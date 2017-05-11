#!/bin/bash

CMD='bin/Rscript rdt-plugins/promises/R/benchmark.R'

PACKAGES=

if [ $# -ge 1 ]
then
    PACKAGES="$@"
else 
    PACKAGES="grid dplyr ggplot2 haven readr readxl stringr tibble tidyverse digest colorspace kernlab vcd rpart survival mclust party mvtnorm igraph"
fi

for i in "$@"
do 
    $CMD $i 2>&1 | tee "$i.log" 
done   

