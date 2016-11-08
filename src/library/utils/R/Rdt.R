RdtTrace <- function(block=NULL, filename="trace.out") {
    stopifnot(is.character(filename) && length(filename) == 1 && nchar(filename) > 0)

    .Call(C_RdtTrace, filename)

    if (!is.null(block)) {
        block
        .Call(C_RdtStop)
    } 
}

RdtStop <- function() {
    .Call(C_RdtStop)    
}
