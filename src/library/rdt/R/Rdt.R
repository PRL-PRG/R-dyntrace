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
    
    .Call(C_Rdt, tracer, environment(), list(...))
}

trace.promises.r <- function(expression, tracer="promises", output="p", format="trace", pretty.print=TRUE, overwrite=FALSE, synthetic.call.id=TRUE, path="trace", include.configuration=FALSE)
    Rdt(expression, tracer=tracer, output=output, path=path, format=format, pretty.print=pretty.print, synthetic.call.id=synthetic.call.id, overwrite=overwrite, include.configuration=include.configuration)

trace.promises.file <- function(expression, tracer="promises", output="f", path="trace.txt", format="trace", pretty.print=FALSE, overwrite=FALSE, synthetic.call.id=TRUE, include.configuration=FALSE)
    Rdt(expression, tracer=tracer, output=output, path=path, format=format, pretty.print=pretty.print, synthetic.call.id=synthetic.call.id, overwrite=overwrite, include.configuration=include.configuration)

trace.promises.sql <- function(expression, tracer="promises", output="f", path="trace.sql", format="sql", pretty.print=FALSE, overwrite=FALSE, synthetic.call.id=TRUE, include.configuration=TRUE)
    Rdt(expression, tracer=tracer, output=output, path=path, format=format, pretty.print=pretty.print, synthetic.call.id=synthetic.call.id, overwrite=overwrite, include.configuration=include.configuration)

trace.promises.compiled.db <- function(expression, tracer="promises", output="d", path="trace.sqlite", format="psql", pretty.print=FALSE, overwrite=FALSE, synthetic.call.id=TRUE, include.configuration=TRUE)
    Rdt(expression, tracer=tracer, output=output, path=path, format=format, pretty.print=pretty.print, synthetic.call.id=synthetic.call.id, overwrite=overwrite, include.configuration=include.configuration)

trace.promises.uncompiled.db <- function(expression, tracer="promises", output="d", path="trace.sqlite", format="sql", pretty.print=FALSE, overwrite=FALSE, synthetic.call.id=TRUE, include.configuration=TRUE)
    Rdt(expression, tracer=tracer, output=output, path=path, format=format, pretty.print=pretty.print, synthetic.call.id=synthetic.call.id, overwrite=overwrite, include.configuration=include.configuration)

trace.promises.db <-trace.promises.compiled.db

trace.promises.both <- function(expression, tracer="promises", output="pd", path="trace.sqlite", format="both", pretty.print=FALSE, overwrite=FALSE, synthetic.call.id=TRUE, include.configuration=TRUE)
    Rdt(expression, tracer=tracer, output=output, path=path, format=format, pretty.print=pretty.print, synthetic.call.id=synthetic.call.id, overwrite=overwrite, include.configuration=include.configuration)

run.all.vignettes.from.package <- function(package, executor = eval, ...) {
    result.set <- vignette(package = package)
    vignettes.in.package <- result.set$results[,3]
    for (v in vignettes.in.package) {
        error.handler = function(err) {
            write(paste("Error in vignette ", v, " for package ", package, ": ",
            as.character(err), sep = ""), file = stderr())
        }

        one.vignette <- vignette(v, package = package)
        R.code.path <- paste(one.vignette$Dir, "doc", one.vignette$R, sep="/")
        R.code.source <- parse(R.code.path)
        tryCatch(executor(R.code.source, ...), error = error.handler)
    }
}

run.all.vignettes.from.packages <- function(packages, executor = eval, ...)
invisible(lapply(packages, function(x) run.all.vignettes.from.package(x, eval, ...)))

run.all.vignettes.from.all.packages <- function(executor = eval, ...)
run.all.vignettes.from.packages(unique(vignette()$results[,1]), executor = eval, ...)

# keep an empty line below this one
