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

packages <- read.csv("r-package-anthology.csv", 
                        header=TRUE, sep=";", 
                        comment.char="#", strip.white=TRUE)

install_my_packages <- function(packages, src, installer=install.packages) {
    for (package in packages) {
        write(paste("Instaling: ", package, " (", src, ")", sep=""), stderr())
        
        if(eval(parse(text=paste("require(", package, ")")))) {
            write("  skipping, already installed", stderr())
            next
        }
    
        write(paste("Instaling:", package, "(CRAN)"), stderr())
        installer(package)
    }
}

cran.packages = (packages[packages$source == "cran", ])$package
bioc.packages = (packages[packages$source == "bioc", ])$package
 aux.packages = c(
    "Rcpp",
    "dplyr",
    "tibble",
    "limma",
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
    "coin",
    "MASS",
    "fBasics",
    "fGarch",
    "SparseM",
    "quantreg",
    "splines",
    "mlbench",
    "DAAGbio", 
    "oz",
    "randomForest",
    "rpart",
    "ape",
    "HSAUR",
    "xtable",
    "KernSmooth",
    "hgu95av2.db",
    "rae230a.db",
    "hom.Hs.inp.db",
    "BiocStyle",
    "Rsamtools",
    "hgu95av2probe",
    "ShortRead",
    "hgu95av2cdf",
    "graph",
    "VariantAnnotation",
    "BSgenome",
    "pasillaBamSubset",
    "TxDb.Dmelanogaster.UCSC.dm3.ensGene",
    "airway",
    "rae230aprobe",
    "org.Mm.eg.db", 
    "affy",
    "affydata",
    "TxDb.Hsapiens.UCSC.hg19.knownGene",
    "BSgenome.Mmusculus.UCSC.mm10",
    "BSgenome.Celegans.UCSC.ce2",
    'TxDb.Mmusculus.UCSC.mm10.knownGene',
    'BatchJobs',
    'RNAseqData.HNRNPC.bam.chr14',
    'TxDb.Athaliana.BioMart.plantsmart22',
    "GO.db",
    "MASS",
    "Biostrings",
    "optparse"
)

source("https://bioconductor.org/biocLite.R")
biocLite()
install_my_packages(aux.packages, "auxiliary", biocLite)
install_my_packages(cran.packages, "CRAN")
install_my_packages(bioc.packages, "bioconductor", biocLite)

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


for (package in packages$package) { 
    eval_all_vignettes_from_package(package) 
    #:readline(prompt="Press [enter] to continue")
}

