# parameters for "promises" tracer:
# - path: default="tracer.db"
# - format: default="trace" other: "SQL"/"sql", "both"
# - output: default="R" other: "file", "DB"/"db", "R+DB"/"DB+R"/"r+db"/"db+r" note: file/DB path indicated by output_path
# - pretty.print: default=TRUE, other: FALSE
# - synthetic.call.id: default=TRUE, other: FALSE
# - overwrite: default=TRUE, other: FALSE
Rdt <- function(block, tracer="promises", ...) {
    stopifnot(is.character(tracer) && length(tracer) == 1 && nchar(tracer) > 0)
    if (missing(block)) stop("block is required")
    
    .Call(C_Rdt, tracer, environment(), list(...))
}

trace.promises.r <- function(expression, tracer="promises", output="R", format="trace", pretty.print=TRUE, overwrite=FALSE, synthetic.call.id=TRUE, path="trace")
    Rdt(expression, tracer=tracer, output=output, path=path, format=format, pretty.print=pretty.print, synthetic.call.id=synthetic.call.id, overwrite=overwrite)

trace.promises.file <- function(expression, tracer="promises", output="file", path="trace.txt", format="trace", pretty.print=FALSE, overwrite=TRUE, synthetic.call.id=TRUE)
    Rdt(expression, tracer=tracer, output=output, path=path, format=format, pretty.print=pretty.print, synthetic.call.id=synthetic.call.id, overwrite=overwrite)

trace.promises.sql <- function(expression, tracer="promises", output="file", path="trace.sql", format="sql", pretty.print=FALSE, overwrite=TRUE, synthetic.call.id=TRUE)
    Rdt(expression, tracer=tracer, output=output, path=path, format=format, pretty.print=pretty.print, synthetic.call.id=synthetic.call.id, overwrite=overwrite)

trace.promises.db <- function(expression, tracer="promises", output="DB", path="trace.sqlite", format="sql", pretty.print=FALSE, overwrite=TRUE, synthetic.call.id=TRUE)
    Rdt(expression, tracer=tracer, output=output, path=path, format=format, pretty.print=pretty.print, synthetic.call.id=synthetic.call.id, overwrite=overwrite)

trace.promises.both <- function(expression, tracer="promises", output="R+DB", path="trace.sqlite", format="both", pretty.print=FALSE, overwrite=TRUE, synthetic.call.id=TRUE)
    Rdt(expression, tracer=tracer, output=output, path=path, format=format, pretty.print=pretty.print, synthetic.call.id=synthetic.call.id, overwrite=overwrite)

# keep an empty line below this one
