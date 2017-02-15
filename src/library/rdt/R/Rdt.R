Rdt <- function(block, tracer="promises", ...) {
    stopifnot(is.character(tracer) && length(tracer) == 1 && nchar(tracer) > 0)
    if (missing(block)) stop("block is required")
    
    .Call(C_Rdt, tracer, environment(), list(...))
}

Rdt_deparse <- function(expression) {
    if (missing(expression)) stop("expression is required")
    .Call(C_Rdt_deparse, expression)
}

