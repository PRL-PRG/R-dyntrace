packages = commandArgs(trailingOnly = TRUE)

cmd <- "~/workspace/R-dyntrace/bin/R CMD BATCH" #paste(shQuote(file.path(R.home("bin"), "R")))
sys.env <- as.character(c("R_KEEP_PKG_SOURCE=yes", "R_ENABLE_JIT=0"))

output.dir <- "/data/kondziu/"
tmp.dir <- "/data/kondziu/"

#execute.external.programs(programs)

instrumented.code.dir <- paste(tmp.dir, "doc", sep="/")
dir.create(instrumented.code.dir, showWarnings = TRUE)

log.dir <- paste(tmp.dir, "log", sep="/")
dir.create(log.dir, showWarnings = TRUE)

rdt.cmd.head <- function(first, path)
  paste(
    "Rdt(tracer='promises',\n",
    "output='d',\n", 
    "path='", path, "',\n", 
    "format='psql',\n",
    "pretty.print=FALSE,\n",
    "overwrite=", first, ",\n", 
    "synthetic.call.id=TRUE,\n", 
    "include.configuration=TRUE,\n",
    "reload.state=", !first, ",\n",
    "block={\n\n",
    sep="")

rdt.cmd.tail <- "\n})"

instrument.vignettes <- function(packages) {
  i.packages <- 0
  n.packages <- length(packages)
  
  instrumented.vignette.paths <- list()
  
  for (package in packages) {
    i.packages <- i.packages + 1
    
    write(paste("[", i.packages, "/", n.packages, "] Instrumenting vignettes for package: ", package, sep=""), stdout())
    
    result.set <- vignette(package = package)
    vignettes.in.package <- result.set$results[,3]
    
    tracer.output.path <- paste(output.dir, "/", package, ".sqlite", sep="")
    
    i.vignettes = 0
    n.vignettes = length(vignettes.in.package)
    
    for (vignette.name in vignettes.in.package) {
      i.vignettes <- i.vignettes + 1
      
      write(paste("[", i.packages, "/", n.packages, "::", i.vignettes, "/", n.vignettes, "] Instrumenting vignette: ", vignette.name, " from ", package, sep=""), stdout())
      
      one.vignette <- vignette(vignette.name, package = package)
      vignette.code.path <- paste(one.vignette$Dir, "doc", one.vignette$R, sep="/")
      instrumented.code.path <- paste(instrumented.code.dir, "/", i.packages, "-", i.vignettes, "_", package, "_", vignette.name, ".R", sep="")
      
      write(paste("[", i.packages, "/", n.packages, "::", i.vignettes, "/", n.vignettes, "] Writing vignette to: ", instrumented.code.path, sep=""), stdout())

      vignette.code <- readLines(vignette.code.path)
      instrumented.code <- c(rdt.cmd.head(i.vignettes == 1, tracer.output.path), vignette.code, rdt.cmd.tail)      
      write(instrumented.code, instrumented.code.path)
      
      write(paste("[", i.packages, "/", n.packages, "::", i.vignettes, "/", n.vignettes, "] Done instrumenting vignette: ", vignette.name, " from ", package, sep=""), stdout())
      
      instrumented.vignette.paths[[i.packages + i.vignettes -1 ]] <- c(package, vignette.name, instrumented.code.path)
    }
    
    write(paste("[", i.packages, "/", n.packages, "] Done vignettes for package: ", package, sep=""), stdout())
  }
  
  instrumented.vignette.paths
}

instrument.and.aggregate.vignettes <- function(packages) {
  i.packages <- 0
  n.packages <- length(packages)
  
  instrumented.vignette.paths <- list()
  
  for (package in packages) {
    i.packages <- i.packages + 1
    
    write(paste("[", i.packages, "/", n.packages, "] Instrumenting vignettes for package: ", package, sep=""), stdout())
    
    result.set <- vignette(package = package)
    vignettes.in.package <- result.set$results[,3]
    
    instrumented.code.path <- paste(instrumented.code.dir, "/", i.packages, "_", package, ".R", sep="")
    tracer.output.path <- paste(output.dir, "/", package, ".sqlite", sep="")
    
    i.vignettes = 0
    n.vignettes = length(vignettes.in.package)
    
    write(rdt.cmd.head(i.vignettes == 1, tracer.output.path), instrumented.code.path, append=FALSE)

    for (vignette.name in vignettes.in.package) {
      i.vignettes <- i.vignettes + 1
      
      write(paste("[", i.packages, "/", n.packages, "::", i.vignettes, "/", n.vignettes, "] Appending vignette: ", vignette.name, " from ", package, sep=""), stdout())
      
      one.vignette <- vignette(vignette.name, package = package)
      vignette.code.path <- paste(one.vignette$Dir, "doc", one.vignette$R, sep="/")
      
      write(paste("[", i.packages, "/", n.packages, "::", i.vignettes, "/", n.vignettes, "] Appending vignette to: ", instrumented.code.path, sep=""), stdout())
      
      vignette.code <- readLines(vignette.code.path)
      write(vignette.code, instrumented.code.path, append=TRUE)
      
      write(paste("[", i.packages, "/", n.packages, "::", i.vignettes, "/", n.vignettes, "] Done appending vignette: ", vignette.name, " from ", package, sep=""), stdout())
    }
    
    write(rdt.cmd.tail, instrumented.code.path, append=TRUE)
    instrumented.vignette.paths[[i.packages]] <- c(package, "all_vignettes", instrumented.code.path)
    
    write(paste("[", i.packages, "/", n.packages, "] Done vignettes for package: ", package, sep=""), stdout())
  }
  
  instrumented.vignette.paths
}

execute.external.programs <- function(program.list, new.process=FALSE) {
  i.programs <- 0
  n.programs <- length(program.list)
  
  for(program in program.list) {
    i.programs <- i.programs + 1
    
    package.name <- program[1]
    vignette.name <- program[2]
    program.path <- program[3]
    
    write(paste("[", i.programs, "/", n.programs, "] Executing file: ", program.path, sep=""), stdout())
    
    log.out.path <- paste(log.dir, "/", i.programs, "_", package.name, "_", vignette.name, ".out", sep="")
    log.err.path <- paste(log.dir, "/", i.programs, "_", package.name, "_", vignette.name, ".err", sep="")
    
    if(new.process) {
      cmd.with.args <- paste(cmd, program.path, log.out.path, log.err.path, sep=" ")
      system2(cmd.with.args, env=sys.env, wait=TRUE)
      write(cmd.with.args, stdout())
    } else {
      source(program.path, local=new.env()) #local=attach(NULL))
    }
    
    write(paste("[", i.programs, "/", n.programs, "] Done executing file: ", program.path, sep=""), stdout())
  }
}

run <- function(..., separately=TRUE) 
  execute.external.programs((if(separately) instrument.vignettes else instrument.and.aggregate.vignettes)(list(...)))
  
if (length(packages) > 0)
  execute.external.programs(instrument.vignettes(packages), new.process = FALSE)
