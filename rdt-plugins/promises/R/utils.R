# This now seems like a horrible idea... we could use a namespace or something.
FILE <- 'f'
CONSOLE <- 'p'
DB <- 'd'

format.output <- function(outputs) do.call(paste, c(as.list(outputs), sep=""))

# TODO rename to trace_promises_r
trace.promises.r <- function(expression, tracer="promises", output=CONSOLE, format="trace", pretty.print=TRUE, overwrite=FALSE, synthetic.call.id=TRUE, path="trace", include.configuration=FALSE, reload.state=FALSE)
    Rdt(expression, tracer=tracer, output=format.output(output), path=path, format=format, pretty.print=pretty.print, synthetic.call.id=synthetic.call.id, overwrite=overwrite, include.configuration=include.configuration, reload.state=reload.state)

# TODO rename to trace_promises_file
trace.promises.file <- function(expression, tracer="promises", output=FILE, path="trace.txt", format="trace", pretty.print=FALSE, overwrite=FALSE, synthetic.call.id=TRUE, include.configuration=FALSE, reload.state=FALSE)
    Rdt(expression, tracer=tracer, output=format.output(output), path=path, format=format, pretty.print=pretty.print, synthetic.call.id=synthetic.call.id, overwrite=overwrite, include.configuration=include.configuration, reload.state=reload.state)

# TODO rename to trace_promises_sql
trace.promises.sql <- function(expression, tracer="promises", output=FILE, path="trace.sql", format="sql", pretty.print=FALSE, overwrite=FALSE, synthetic.call.id=TRUE, include.configuration=TRUE, reload.state=FALSE)
    Rdt(expression, tracer=tracer, output=format.output(output), path=path, format=format, pretty.print=pretty.print, synthetic.call.id=synthetic.call.id, overwrite=overwrite, include.configuration=include.configuration, reload.state=reload.state)

# TODO rename to trace_promises_compiled_db
trace.promises.compiled.db <- function(expression, tracer="promises", output=DB, path="trace.sqlite", format="psql", pretty.print=FALSE, overwrite=FALSE, synthetic.call.id=TRUE, include.configuration=TRUE, reload.state=FALSE)
    Rdt(expression, tracer=tracer, output=format.output(output), path=path, format=format, pretty.print=pretty.print, synthetic.call.id=synthetic.call.id, overwrite=overwrite, include.configuration=include.configuration, reload.state=reload.state)

# TODO rename to trace_promises_uncompiled_db
trace.promises.uncompiled.db <- function(expression, tracer="promises", output=DB, path="trace.sqlite", format="sql", pretty.print=FALSE, overwrite=FALSE, synthetic.call.id=TRUE, include.configuration=TRUE, reload.state=FALSE)
    Rdt(expression, tracer=tracer, output=format.output(output), path=path, format=format, pretty.print=pretty.print, synthetic.call.id=synthetic.call.id, overwrite=overwrite, include.configuration=include.configuration, reload.state=reload.state)

# TODO rename to trace_promises_db
trace.promises.db <- trace.promises.compiled.db

# TODO rename to trace_promises_both
trace.promises.both <- function(expression, tracer="promises", output=c(CONSOLE, DATABASE), path="trace.sqlite", format="both", pretty.print=FALSE, overwrite=FALSE, synthetic.call.id=TRUE, include.configuration=TRUE, reload.state=FALSE)
    Rdt(expression, tracer=tracer, output=format.output(output), path=path, format=format, pretty.print=pretty.print, synthetic.call.id=synthetic.call.id, overwrite=overwrite, include.configuration=include.configuration, reload.state=reload.state)

library(dplyr)

get_trace_call_graph <- function(path="trace.sqlite") tbl(src_sqlite(path), "out_call_graph")
get_trace_strictness <- function(path="trace.sqlite") tbl(src_sqlite(path), "out_strictness")
get_trace_force_order <- function(path="trace.sqlite") tbl(src_sqlite(path), "out_force_order")

get_function_by_id <- function(function_id, path="trace.sqlite")
    filter(tbl(src_sqlite(path), "functions"), id == function_id)

get_function_aliases_by_id <- function(id, path="trace.sqlite")
    filter(tbl(src_sqlite(path), "function_names"), function_id == id)