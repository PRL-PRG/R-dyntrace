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

Rdt_deparse <- function(expression) {
    if (missing(expression)) stop("expression is required")
    .Call(C_Rdt_deparse, expression)
}

