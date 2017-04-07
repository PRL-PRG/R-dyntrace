# parameters for "promises" tracer:
# - path: default="tracer.db"
# - format: default="trace" other: "SQL"/"sql", "both", "PSQL/psql"
# - output: default="R" other: "file", "DB"/"db", "R+DB"/"DB+R"/"r+db"/"db+r" note: file/DB path indicated by output_path
# - pretty.print: default=TRUE, other: FALSE
# - synthetic.call.id: default=TRUE, other: FALSE
# - overwrite: default=TRUE, other: FALSE
Rdt <- function(block, tracer="promises", ...) {
    stopifnot(is.character(tracer) && length(tracer) == 1 && nchar(tracer) > 0)
    if (missing(block)) stop("block is required")

    start.time <- Sys.time()

    retval <- .Call(C_Rdt, tracer, environment(), list(...))

    end.time <- Sys.time()
    write(paste("Elapsed time:", (end.time - start.time)), stderr())

    retval
}

# This now seems like a horrible idea... we could use a namespace or something.
FILE <- 'f'
CONSOLE <- 'p'
DB <- 'd'

format.output <- function(outputs) do.call(paste, c(as.list(outputs), sep=""))

trace.promises.r <- function(expression, tracer="promises", output=CONSOLE, format="trace", pretty.print=TRUE, overwrite=FALSE, synthetic.call.id=TRUE, path="trace", include.configuration=FALSE)
    Rdt(expression, tracer=tracer, output=format.output(output), path=path, format=format, pretty.print=pretty.print, synthetic.call.id=synthetic.call.id, overwrite=overwrite, include.configuration=include.configuration)

trace.promises.file <- function(expression, tracer="promises", output=FILE, path="trace.txt", format="trace", pretty.print=FALSE, overwrite=FALSE, synthetic.call.id=TRUE, include.configuration=FALSE)
    Rdt(expression, tracer=tracer, output=format.output(output), path=path, format=format, pretty.print=pretty.print, synthetic.call.id=synthetic.call.id, overwrite=overwrite, include.configuration=include.configuration)

trace.promises.sql <- function(expression, tracer="promises", output=FILE, path="trace.sql", format="sql", pretty.print=FALSE, overwrite=FALSE, synthetic.call.id=TRUE, include.configuration=TRUE)
    Rdt(expression, tracer=tracer, output=format.output(output), path=path, format=format, pretty.print=pretty.print, synthetic.call.id=synthetic.call.id, overwrite=overwrite, include.configuration=include.configuration)

trace.promises.compiled.db <- function(expression, tracer="promises", output=DB, path="trace.sqlite", format="psql", pretty.print=FALSE, overwrite=FALSE, synthetic.call.id=TRUE, include.configuration=TRUE)
    Rdt(expression, tracer=tracer, output=format.output(output), path=path, format=format, pretty.print=pretty.print, synthetic.call.id=synthetic.call.id, overwrite=overwrite, include.configuration=include.configuration)

trace.promises.uncompiled.db <- function(expression, tracer="promises", output=DB, path="trace.sqlite", format="sql", pretty.print=FALSE, overwrite=FALSE, synthetic.call.id=TRUE, include.configuration=TRUE)
    Rdt(expression, tracer=tracer, output=format.output(output), path=path, format=format, pretty.print=pretty.print, synthetic.call.id=synthetic.call.id, overwrite=overwrite, include.configuration=include.configuration)

trace.promises.db <- trace.promises.compiled.db

trace.promises.both <- function(expression, tracer="promises", output=c(CONSOLE, DATABASE), path="trace.sqlite", format="both", pretty.print=FALSE, overwrite=FALSE, synthetic.call.id=TRUE, include.configuration=TRUE)
    Rdt(expression, tracer=tracer, output=format.output(output), path=path, format=format, pretty.print=pretty.print, synthetic.call.id=synthetic.call.id, overwrite=overwrite, include.configuration=include.configuration)

wrap.executor <- function(executor)
    function(expr, current_vignette, total_vignettes, vignette_name, vignette_package, ...) {
        write(paste("Vignette ", current_vignette, "/", (total_vignettes - 1),
                        " (", vignette_name, " from ", vignette_package, ")", sep=""),
            stderr())

        executor(eval(expr), ...)
    }

wrap.session.executor <- function(executor)
    function(expr, current_vignette, total_vignettes, vignette_name, vignette_package, ...) {
        write(paste("Vignette ", current_vignette, "/", (total_vignettes - 1),
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
