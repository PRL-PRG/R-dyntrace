findPlugins <- function() {
	plugins <- list.files(.RdtPlugins, full.names = TRUE)

	for (plugin in plugins) {
		utils <- file.path(plugin, "R", "utils.R")
		if (file.exists(utils)) {
			source(utils)
		}
	}
}

.onLoad <- function(libname, pkgname) {
	if(exists(".RdtPlugins")) {
		findPlugins()
	}
}

Rdt <- function(block, tracer="default", ...) {
    stopifnot(is.character(tracer) && length(tracer) == 1 && nchar(tracer) > 0)
    if (missing(block)) stop("block is required")

    start.time <- Sys.time()

    retval <- .Call(C_Rdt, tracer, environment(), list(...))

    end.time <- Sys.time()
    write(paste("Elapsed time:", (end.time - start.time)), stderr())

    retval
}


wrap.executor <- function(executor)
    function(expr, current_vignette, total_vignettes, vignette_name, vignette_package, ...) {
        write(paste("Vignette ", (current_vignette + 1), "/", total_vignettes,
                        " (", vignette_name, " from ", vignette_package, ")", sep=""),
            stderr())

        executor(eval(expr), ...)
    }

wrap.session.executor <- function(executor)
    function(expr, current_vignette, total_vignettes, vignette_name, vignette_package, ...) {
        write(paste("Vignette ", (current_vignette + 1), "/", total_vignettes,
                        " (", vignette_name, " from ", vignette_package, ")", sep=""),
            stderr())

        executor(eval(expr), overwrite=if (current_vignette == 0) TRUE else FALSE, ...)
    }

wrap.contination.executor <- function(executor)
    function(expr, current_vignette, total_vignettes, vignette_name, vignette_package, ...) {
        write(paste("Vignette ", (current_vignette + 1), "/", total_vignettes,
                        " (", vignette_name, " from ", vignette_package, ")", sep=""),
            stderr())

        executor(eval(expr), overwrite=FALSE, reload.state=TRUE, ...)
    }

list.vignettes.in.package <- function (package) vignette(package=package)$results[,3]

run.one.vignette.from.package <- function(package, v, executor = wrap.executor(Rdt) , current_vignette=0, total_vignettes=1, ...) {
    error.handler = function(err) {
        write(paste("Error in vignette ", v, " for package ", package, ": ",
        as.character(err), sep = ""), file = stderr())
    }

    one.vignette <- vignette(v, package = package)
    R.code.path <- paste(one.vignette$Dir, "doc", one.vignette$R, sep="/")
    R.code.source <- parse(R.code.path)
    tryCatch(
        executor(
            R.code.source,
                current_vignette=current_vignette,
                total_vignettes=total_vignettes,
                vignette_name=v,
                vignette_package=package,
            ...),
        error = error.handler
    )
}

run.all.vignettes.from.package <- function(package, executor = wrap.executor(Rdt) , ...) {
    result.set <- vignette(package = package)
    vignettes.in.package <- result.set$results[,3]
    index = 0
    total = length(vignettes.in.package)

    for (v in vignettes.in.package) {
        run.one.vignette.from.package(package, v, executor=executor, current_vignette=index, total_vignettes=total)
        index <- index + 1
    }
}

run.all.vignettes.from.packages <- function(packages, executor = wrap.executor(Rdt), ...)
    invisible(lapply(packages, function(x) run.all.vignettes.from.package(x, eval, ...)))

run.all.vignettes.from.all.packages <- function(executor = wrap.executor(Rdt), ...)
    run.all.vignettes.from.packages(unique(vignette()$results[,1]), executor = eval, ...)

# keep an empty line below this one
