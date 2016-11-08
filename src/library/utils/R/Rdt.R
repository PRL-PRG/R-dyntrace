RdtFlowInfo <- function(filename="flowinfo.out", ...) {
    .Call(C_RdtFlowInfo, list(filename=filename, ...))
}

RdtTrace <- function(filename="trace.out",...) {
    .Call(C_RdtTrace, list(filename=filename, ...))
}
