calltimes <- function(filename="trace.out") {
  data <- read.csv(filename)

  ctx <- new.env()
  ctx$timestamp=0
  ctx$depth=1
  ctx$exclude=list(0)
  ctx$fun_ts=list()
  ctx$counts=list()
  ctx$times_incl=list()
  ctx$times_excl=list()

  entries <- c("function-entry", "bc-function-entry", "builtin-entry")
  exits <- c("function-exit", "bc-function-exit", "builtin-exit")

  entry <- function(name) {
    ctx$depth <- ctx$depth + 1
    ctx$exclude[ctx$depth] <- 0
    ctx$fun_ts[ctx$depth] <- ctx$timestamp

    if (!(name %in% names(ctx$counts))) ctx$counts[name] <- 0
    if (!(name %in% names(ctx$times_incl))) ctx$times_incl[name] <- 0
    if (!(name %in% names(ctx$times_excl))) ctx$times_excl[name] <- 0
  }

  exit <- function(name) {
    elapsed_incl <- ctx$timestamp - ctx$fun_ts[[ctx$depth]]
    elapsed_excl <- elapsed_incl - ctx$exclude[[ctx$depth]]

    ctx$fun_ts[ctx$depth] <- 0
    ctx$exclude[ctx$depth] <- 0

    ctx$counts[name] <- ctx$counts[[name]] + 1
    ctx$times_incl[name] <- ctx$times_incl[[name]] + elapsed_incl
    ctx$times_excl[name] <- ctx$times_excl[[name]] + elapsed_excl

    ctx$depth <- ctx$depth - 1
    ctx$exclude[ctx$depth] <- ctx$exclude[[ctx$depth]] + elapsed_incl
  }

  for (i in 1:nrow(data)) {
    delta = data[i, "DELTA"]
    type = as.character(data[i, "TYPE"])
    name = as.character(data[i, "NAME"])

    ctx$timestamp <- ctx$timestamp + delta

    if (type %in% entries) {
      entry(name)
    } else if (type %in% exits && ctx$depth != 1) {
      exit(name)
    }
  }

  ctx$counts["TOTAL"] <- sum(unlist(ctx$counts))
  ctx$times_excl["TOTAL"] <- sum(unlist(ctx$times_excl))

  # convert to DF
  df_counts <- as.data.frame(cbind(names(ctx$counts), unlist(ctx$counts, use.names=F)))
  colnames(df_counts) <- c("NAME", "COUNT")
  df_times_incl <- as.data.frame(cbind(names(ctx$times_incl), unlist(ctx$times_incl, use.names=F)))
  colnames(df_times_incl) <- c("NAME", "T_INCL")
  df_times_excl <- as.data.frame(cbind(names(ctx$times_excl), unlist(ctx$times_excl, use.names=F)))
  colnames(df_times_excl) <- c("NAME", "T_EXCL")

  # merge
  tmp = merge(df_counts, df_times_incl, by="NAME")
  merge(tmp, df_times_excl, by="NAME")
}
