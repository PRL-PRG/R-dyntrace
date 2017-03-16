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

trace.promises.r <- function(expression)
    Rdt(expression, tracer="promises", output="R", format="trace", pretty.print=TRUE)
trace.promises.file <- function(expression, path="trace.txt")
    Rdt(expression, tracer="promises", output="file", path=path, format="trace", pretty.print=FALSE, overwrite=TRUE)
trace.promises.sql <- function(expression, path="trace.sql")
    Rdt(expression, tracer="promises", output="file", path=path, format="sql", pretty.print=FALSE, overwrite=TRUE)
trace.promises.db <- function(expression, path="trace.sqlite")
    Rdt(expression, tracer="promises", output="db", path=path, format="sql", pretty.print=FALSE, overwrite=TRUE)

trace.promises.both <- function(expression, path="trace.sqlite")
	Rdt(expression, tracer="promises", output="R+DB", format="both", path=path, overwrite=TRUE)
# keep an empty line below this one
