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

# requires dplyr, igraph, RSQLite

library(dplyr)
library(igraph)

get_trace_call_graph <- function(path="trace.sqlite")
    src_sqlite(path) %>% tbl("out_call_graph")

get_trace_strictness <- function(path="trace.sqlite")
    src_sqlite(path) %>% tbl("out_strictness")

get_trace_force_order <- function(path="trace.sqlite")
    src_sqlite(path) %>% tbl("out_force_order")

get_function_by_id <- function(function_id, path="trace.sqlite")
    src_sqlite(path) %>% tbl("functions") %>% filter(id == function_id)

get_function_aliases_by_id <- function(id, path="trace.sqlite")
    src_sqlite(path) %>% tbl("function_names") %>% filter(function_id == id)

# db <- src_sqlite(path)
get_trace_call_graph_as_igraph <- function(db) {
    # 1. make define edges for call graph
    cg <-
        # get data from database
        db %>% tbl("out_call_graph") %>%
        # select only the two columns we need
        select(caller_function, callee_function) %>%
        # flatten to an edge list -- this could be simply `apply(1, c) %>% c` if all nodes > 0, else convert to string
        as.data.frame %>% apply(1, function(x) c(toString(x[1]), toString(x[2]))) %>% c %>%
        # construct graph from edge list
        make_directed_graph

    # 2. fill in attributes: aliases
    V(cg)$aliases <-
        # get data from database
        (db %>% tbl("function_names") %>%
        # sort by id, so that it fits the data in the graph
        arrange(function_id) %>%
        # retrieve the column we're interested in
        select(names) %>%
        # convert data structure
        as.data.frame)$names

    # 3. fill in attributes: function types
    V(cg)$type <-
        # get data from database
        (db %>% tbl("functions") %>%
        # convert types to strings
        mutate(htype = if (type == 0) 'closure' else
                       if (type == 1) 'built-in' else
                       if (type == 2) 'special' else NULL) %>%
        # sort by id, so that it fits the data in the graph
        arrange(id) %>%
        # retrieve the column we want
        select(htype) %>%
        # convert data structure
        as.data.frame)$htype

    # 4. color graph by function type
    V(cg)$color <- ifelse(V(cg)$type == "closure", "green", "red")

    # Finally, return graph
    cg
}

calculate_distance <- function(cg, form, to) {
    shortest_paths(cg, from=from, to=to)$vpath
}

function(db) {
    prom_eval <-
        # make an outer join to get evaluated and unevaluated promises
        db %>% tbl(sql("select * from promises left outer join promise_evaluations on id = promise_id")) %>%
        # filter out lookups, leave NAs and forces
        filter(is.na(event_type) || event_type == 15) %>%
        #      1    2           3             4
        select(id, event_type, from_call_id, in_call_id) %>%
#        mutate(locality = if(is.na(promise_id)) "virgin" else
#                          if(
        as.data.frame

    classify <- function(row) {
        if (is.na(row[2]))
            "unclassified"
        else {
            created <- function_from_call(db, row[3]) # maybe this can be moved to select FIXME
            forced <- function_from_call(db, row[4]) # FIXME to implement

            distance <- calculate_distance(cg, created, forced) # FIXME to implement

            if (is.na(distance))
                "escaped"
            else if (distance == 0)
                "call-local"
            else
                "branch-local"
        }
    }

    prom_eval$classification <- prom_eval %>% apply(1, classify)


    #db %>% tbl("promise_evaluations") %>% filter(event_type != 0) %>% select(promise_id, from_call_id, in_call_id)
    prom_eval
}
#> src_sqlite(path) %>% tbl("promise_evaluations") %>% filter(event_type == 15) %>% select(promise_id, from_call_id, in_call_id) %>% as.data.frame %>% apply(1, c )



#shortest_paths(cg, from="0", to="7")$vpath
