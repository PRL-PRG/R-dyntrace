RdtTrace({
  a <- function () { Sys.sleep(1) }
  b <- function () { a(); Sys.sleep(1) }
  b()
})
