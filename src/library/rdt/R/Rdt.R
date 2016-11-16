RdtTrace <- function(block, filename=NULL, disabled_probes=NULL) {
    if (!is.null(filename)) stopifnot(is.character(filename) && length(filename) == 1 && nchar(filename) > 0)

    if (!missing(block)) {
        .Call(C_RdtTraceBlock, environment(), filename, disabled_probes)
    } else {
        .Call(C_RdtTrace, filename, disabled_probes)
    }
}

RdtNoop <- function(block=NULL) {
    .Call(C_RdtNoop)
    
    if (!is.null(block)) {
        block
        .Call(C_RdtNoop)
    }    
}
