# Run as: Rscript file argv

DIR="/data/kondziu/"

packages = commandArgs(trailingOnly = TRUE)

custom.executor <- function(executor, path)
    function(expr, current_vignette, total_vignettes, vignette_name, vignette_package, ...) {
        write(paste("Vignette ", (current_vignette + 1), "/", total_vignettes,
                        " (", vignette_name, " from ", vignette_package, "), saving to '", path, "'", sep=""),
            stderr())

    executor(eval(expr), path=path, overwrite=if (current_vignette == 0) TRUE else FALSE, ...)
}

#start <- proc.time()

for (p in packages) {
    print(p)
    run.all.vignettes.from.package(p, custom.executor(trace.promises.db, path=paste(DIR, p, ".sqlite", sep="")))
}

#end <- proc.time()

#write("Elapsed time:", stderr())
#write(end - start, stderr())
