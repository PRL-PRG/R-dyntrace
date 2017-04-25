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
	print("Oh hi")
	if(exists(".RdtPlugins")) {
		findPlugins()
	}
}

Rdt <- function(block, tracer="promises", ...) {
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

    executor(eval(expr), overwrite=if (current_vignette == 0) TRUE else FALSE, ...)
}

run.all.vignettes.from.package <- function(package, executor = wrap.executor(trace.promises.r) , ...) {
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
            executor(
                R.code.source,
                current_vignette=index,
                total_vignettes=total,
                vignette_name=v,
                vignette_package=package,
                ...),
            error = error.handler
        )

        index <- index + 1
    }
}

run.all.vignettes.from.packages <- function(packages, executor = wrap.executor(trace.promises.r), ...)
    invisible(lapply(packages, function(x) run.all.vignettes.from.package(x, eval, ...)))

run.all.vignettes.from.all.packages <- function(executor = wrap.executor(trace.promises.r), ...)
    run.all.vignettes.from.packages(unique(vignette()$results[,1]), executor = eval, ...)

# keep an empty line below this one
