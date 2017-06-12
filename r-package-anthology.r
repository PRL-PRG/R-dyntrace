# Other things that need to be installed:
# - GSL (GNU Scientific Library)
#   sudo apt install gsl-bin libgsl-dbg libgsl-dev
# - XML2
#   sudo apt install xml2 libxml2-dev
# - Set environment:
#   sudo apt-get install xvfb
#   nohup Xvfb :6 -screen 0 1280x1024x24 >/dev/null 2>&1
#   export DISPLAY=:6

# run as xvfb-run bin/R
Sys.setenv("DISPLAY"=":0")

library(dplyr)

packages <- read.csv("r-package-anthology.csv", 
                        header=TRUE, sep=";", 
                        comment.char="#", strip.white=TRUE)

# Install CRAN packages
for (package in (packages %>% filter(source == "cran"))$package) {
    write(paste("Instaling:", package, "(CRAN)"), stderr())
    
    if(eval(parse(text=paste("require(", package, ")")))) {
        write("  skipping, already installed", stderr())
        next
    }

    write(paste("Instaling:", package, "(CRAN)"), stderr())
    install.packages(package)
}

# Install additional packages needed by vignettes, but not listed as pkg
# dependencies.
install.packages(c(
#    "knitr",
    "nycflights13",
    "Lahman",
    "chron",
    "tseries",
    "vcd",
    "strucchange",
    "timeDate",
    "sp",
    "leaps",
    "Ecdat",
    "diagram",
    "kernlab", 
    "mlbench",
    "sfsmisc",
    "timeSeries",
    "SMIR",
    "gstat",
    "forecast",
    
    "FitARMA",
    "polynom",
    "expsmooth",
    "RgoogleMaps",

    "Rmpfr",
    "TSA",
    "dse",
))


source("https://bioconductor.org/biocLite.R")
biocLite()
for (package in (packages %>% filter(source == "bioc"))$package) {
    write(paste("Instaling:", package, "(bioconductor)"), stderr())

    if(eval(parse(text=paste("require(", package, ")")))) {
        write("  skipping, already installed", stderr())
        next
    }

    biocLite(package)
}

# Patches for whatever I can patch
# Error in vignette zoo for package zoo: Error in readChar(con, 5L, useBytes = TRUE): cannot open the connection
# Error in vignette sha1 for package digest: Error: sha1() has not method for the 'terms', 'formula' class
# Error in vignette figs6 for package gamclass: Error in eval(model$call$data, envir): object 'meuse' not found
# Error in vignette rhoAMH-dilog for package copula: Error in mat2tex(rbind(k = k, `$a_k$` = as.character(ak)), stdout()): could not find function "mat2tex"
# Error in vignette Frank-Rmpfr for package copula: Error in mpfr(thet, precBits = pBit): could not find function "mpfr"


# sanity check
eval_all_vignettes_from_package <- function(package, ...) {
    result.set <- vignette(package = package)
    vignettes.in.package <- result.set$results[,3]
    index = 0
    total = length(vignettes.in.package)

    for (v in vignettes.in.package) {
        
        error.handler = function(err) {
            write(paste("Error in vignette ", v, " for package ", package, ": ",
            as.character(err), sep = ""), file = stderr())
        }
    
        one.vignette <- vignette(v, package = package)
        R.code.path <- paste(one.vignette$Dir, "doc", one.vignette$R, sep="/")
        R.code.source <- parse(R.code.path)
        tryCatch(
            {
                write(paste("Running vignette ", v, "for package ", package, sep = ""), file = stderr())
                eval(R.code.source)
            },
            error = error.handler
        )

        index <- index + 1
    }
}


for (package in packages$package) { 
    eval_all_vignettes_from_package(package) 
    readline(prompt="Press [enter] to continue")
}

