# Other things that need to be installed:
# - GSL (GNU Scientific Library)
#   sudo apt install gsl-bin libgsl-dbg libgsl-dev
# - XML2
#   sudo apt install xml2 libxml2-dev
# - MPFR
#   sudo apt install libmpfr-dev libmpfr-doc libmpfr4 libmpfr4-dbg

# optional
#   sudo apt-get install xvfb
#   nohup Xvfb :6 -screen 0 1280x1024x24 >/dev/null 2>&1
#   export DISPLAY=:6

# R_MAX_NUM_DLLS=

# run as xvfb-run bin/R
Sys.setenv("DISPLAY"=":0")

# sanity check
eval_all_vignettes_from_package <- function(package) {
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
                write(paste("Running vignette ", v, " for package ", package, sep = ""), file = stderr())
                eval(R.code.source)
            },
            error = error.handler
        )

        index <- index + 1
    }
}

packages = commandArgs(trailingOnly = TRUE)
for (package in packages) { 
    eval_all_vignettes_from_package(package) 
    #:readline(prompt="Press [enter] to continue")
}

