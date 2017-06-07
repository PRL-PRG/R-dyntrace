# requires dplyr, igraph, RSQLite

library(RSQLite)
library(dplyr)
library(igraph)

get_trace_strictness <- function(path="trace.sqlite")
    src_sqlite(path) %>% tbl("out_strictness")

get_trace_force_order <- function(path="trace.sqlite")
    src_sqlite(path) %>% tbl("out_force_order")

get_function_by_id <- function(function_id, path="trace.sqlite")
    src_sqlite(path) %>% tbl("functions") %>% filter(id == function_id)

get_function_aliases_by_id <- function(id, path="trace.sqlite")
-
src_sqlite(path) %>% tbl("function_names") %>% filter(function_id == id)

# Derive a call graph from a trace. This call graph agregates a concrete call tree by translating calls to function
# definitions.
call_graph_from_trace <- function(db) {
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
    V(cg)$label <-
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
        mutate(htype = if (type == 0) "closure" else
                       if (type == 1) "built-in" else
                       if (type == 2) "special" else
                       if (type == 3) "primitive" else NULL) %>%
        # sort by id, so that it fits the data in the graph
        arrange(id) %>%
        # retrieve the column we want
        select(htype) %>%
        # convert data structure
        as.data.frame)$htype

    # 4. color graph by function type
    V(cg)$color <- ifelse(V(cg)$type %in% c("closure", "built-in", NA), "green", "red")

    # 5. weight edges by colors
    E(cg)$weight <- get.edgelist(cg) %>% apply(1, function(edge) if (V(cg)[edge[2]]$color == "red") 0 else 1)

    # Finally, return graph
    cg
}

# Uses a call graph to classify each promise to be either forced locally, relayed, escaped, or not forced.
promise_lifestyle_from_cg <- function(db, cg) {
    promises <- db %>% tbl("promises")
    promise_evaluations <- db %>% tbl("promise_evaluations")
    function_dictionary <- db %>% tbl("calls") %>% select(id, function_id) %>% rename(call_id=id)

    classify <- function(created, forced)
        if (is.na(created) || is.na(forced))
            "virgin"
        else {
            distance <- distances(cg, v=created, to=forced, mode="out")[1]
        if (distance == Inf)
            "escaped"
        else if (distance == 0)
            "local"
        else #if (0 < distance < Inf)
            "vicarious"
        }

    # make an outer join to get evaluated and unevaluated promises
    left_join(promises, promise_evaluations, by=c("id" = "promise_id")) %>%
    # filter out lookups, leave NAs and forces
    filter(is.na(event_type) || event_type == 15) %>%
    # include function ids for in_call_id
    left_join(function_dictionary, by=c("in_call_id" = "call_id")) %>%
    rename(forced_function_id=function_id) %>%
    # include function ids for from_call_id
    left_join(function_dictionary, by=c("from_call_id" = "call_id")) %>%
    rename(created_function_id=function_id) %>%
    # only what we need: promise and function ids
    select(id, created_function_id, forced_function_id) %>%
    # strings!
    mutate(created_function_id=as.character(created_function_id),
           forced_function_id=as.character(forced_function_id)) %>%
    group_by(id) %>% do(mutate(., locality=classify(created_function_id, forced_function_id)))
}

# Derive a concrete call tree from a trace. This is a tree built on actual calls (not functions).
concrete_call_tree_from_trace <- function(db) {
    calls <- db %>% tbl("calls")
    functions <- db %>% tbl("functions")
    all_call_ids <- {
        call_ids <- calls %>% select(id)
        parent_ids <- calls %>% select(parent_id) %>% rename(id=parent_id)
        union_all(call_ids, parent_ids)
    } %>% distinct(id) %>% arrange(id)

    # 1. define edges for call tree
    cct <-
        calls %>% mutate(id.s = as.character(id), parent_id.s = as.character(parent_id)) %>%
        select(parent_id.s, id.s) %>% rename(parent_id=parent_id.s, id=id.s) %>%
        as.data.frame %>% apply(1, c) %>% c %>%
        make_directed_graph

    # 2. fill in attributes: function name
    V(cct)$label <-
        (left_join(all_call_ids, calls, by="id") %>%
        arrange(id) %>%
        select(function_name) %>%
        as.data.frame)$function_name

    # 3. fill in attributes: function types
    V(cct)$type <-
        (left_join(all_call_ids, calls, by="id") %>%
        left_join(functions %>% rename(function_id=id), by="function_id") %>%
        mutate(htype = if (type == 0) "closure" else
                       if (type == 1) "built-in" else
                       if (type == 2) "special" else
                       if (type == 3) "primitive" else NULL) %>%
        select(htype) %>%
        as.data.frame)$htype

    # 4. color tree by function type
    V(cct)$color <- ifelse(V(cct)$type %in% c("closure", "built-in", NA), "green", "red")

    # 5. weight edges by colors
    E(cct)$weight <- get.edgelist(cct) %>% apply(1, function(edge) if (V(cct)[edge[2]]$color == "red") 0 else 1)

    # Finally, return graph
    cct
}

# Uses a concrete call tree to classify each promise to be either forced locally, relayed, escaped, or not forced.
promise_lifestyle_from_cct <- function(db, cct) {
    promises <- db %>% tbl("promises")
    promise_evaluations <- db %>% tbl("promise_evaluations")

    classify <- function(created, forced) {
        if (is.na(created) || is.na(forced))
            "virgin"
        else {
            distance <- distances(cct, v=created, to=forced, mode="out")[1]
            if (distance == Inf)
                "escaped"
            else if (distance == 0)
                "local"
            else #if (0 < distance < Inf)
                "vicarious"
        }
    }

    # by analogy to cg
    left_join(promises, promise_evaluations, by=c("id" = "promise_id")) %>%
    filter(is.na(event_type) || event_type == 15) %>%
    rename(created_call_id=in_call_id) %>%
    rename(forced_call_id=from_call_id) %>%
    select(id, created_call_id, forced_call_id) %>%
    mutate(created_call_id=as.character(created_call_id),
    forced_call_id=as.character(forced_call_id)) %>%
    group_by(id) %>% do(mutate(., locality=classify(created_call_id, forced_call_id)))
}

print_graph <- function(g, file="graph", size=20, labels=NULL) {
    color_function <- function(color) {
        ifelse(color == "red",   "#FFAAAA",
        ifelse(color == "green", "#A5C663",
                                 "#669999"))
    }

    pdf(paste(file, ".pdf", sep=""), height=size, width=size)
    lo <- layout.auto(g)
    plot(0, type="n", ann=FALSE, axes=FALSE, xlim=extendrange(lo[,1]), ylim=extendrange(lo[,2]))
    plot.igraph(g, layout=lo, add=TRUE, rescale=FALSE,
                   edge.arrow.size = .75, edge.curved = seq(-0.3, 0.3, length = ecount(g)),
                   vertex.label=labels,
                   vertex.shape = "rectangle",
                   vertex.color = color_function(V(g)$color),
                   vertex.frame.color = color_function(V(g)$color),
                   vertex.size = (strwidth(V(g)$label) + strwidth("oo")) * 100,
                   vertex.size2 = strheight("I") * 2 * 100)
    dev.off()
}

how_many_promises_are_evaluated_in_the_same_function_in_which_they_are_created <- function() {
    db <- src_sqlite(path)
    cct <- get_trace_concrete_call_tree(db)
    pl <- get_trace_promise_lifespan_for_concrete_call_tree()
    (pl %>% count(classification == "local") %>% as.data.frame)$n
}

where_are_promises_evaluated_cg <- function(path="trace.sqlite") {
    db <- load_trace(path)
    cg <- call_graph_from_trace(db)
    cg.ls <- promise_lifestyle_from_cg(db,cg)
    cg.ls %>% group_by(locality) %>% count(locality)
}

where_are_promises_evaluated_cct <- function(path="trace.sqlite") {
    db <- load_trace(path)
    cct <- concrete_call_tree_from_trace(db)
    cct.ls <- promise_lifestyle_from_cg(db,cct)
    cct.ls %>% group_by(locality) %>% count(locality)
}

#inner_join(cg.ls %>% rename(locality.cg=locality,creat.cg=created_function_id,force.cg=forced_function_id), cct.ls %>% rename(locality.cct=locality,creat.cct=created_function_id,force.cct=forced_function_id), by="id") %>% as.data.frame #%>% select (id, created_call_id, forced_call_id, locality.cg, locality.cct)


# de noveau ###################
load_trace <- function(path="trace.sqlite") src_sqlite(path)

where_are_promises_evaluated <- function(path="trace.sqlite") {
    db <- load_trace(path)
    promise_evaluations <- db %>% tbl("promise_evaluations") %>% filter(event_type == 15) %>% filter(promise_id > 0)
    promises <- db %>% tbl("promises")

    lifestyles <-
        left_join(promises, promise_evaluations, by=c("id" = "promise_id")) %>%
        mutate(lifestyle = ifelse(lifestyle == 1, "local",
                           ifelse(lifestyle == 2, "branch-local",
                           ifelse(lifestyle == 3, "escaped",
                                                  "virgin")))) %>%
        select(id, from_call_id, in_call_id, lifestyle)

    lifestyles %>% group_by(lifestyle) %>% count(lifestyle)
}

what_types_of_promises_are_there <- function(path="trace.sqlite") {
    humanize_type = function(type, fallback_type=NULL)
        if(type == 0) "NIL" else
        if(type == 1) "SYM" else
        if(type == 2) "LIST" else
        if(type == 3) "CLOS" else
        if(type == 4) "ENV" else
        if(type == 5) "PROM" else
        if(type == 6) "LANG" else
        if(type == 7) "SPECIAL" else
        if(type == 8) "BUILTIN" else
        if(type == 9) "CHAR" else
        if(type == 10) "LGL" else
        if(type == 13) "INT" else
        if(type == 14) "REAL" else
        if(type == 15) "CPLX" else
        if(type == 16) "STR" else
        if(type == 17) "DOT" else
        if(type == 18) "ANY" else
        if(type == 19) "VEC" else
        if(type == 20) "EXPR" else
        if(type == 21) {
          if(is.null(fallback_type)) "BCODE"
          else paste("BCODE", Recall(fallback_type), sep=" ")
        } else
        if(type == 22) "EXTPTR" else
        if(type == 23) "WEAKREF" else
        if(type == 24) "RAW" else
        if(type == 25) "S4" else NULL

    db <- load_trace(path)

    promises <- db %>% tbl("promises")

    types <-
        promises %>% #mutate(archetype = type*100+if(is.na(original_type)) 99 else original_type) %>%
        group_by(type, original_type) %>% count(type, original_type) %>% arrange(original_type, type)

    total <- (promises %>% count %>% as.data.frame)$n

    types %>% 
      mutate(percent=((n*100/total))) %>%
      group_by(type, original_type) %>% 
      do(mutate(., type_code = type, type = humanize_type(type, original_type))) %>%
      group_by(type) %>% select(type, n, percent)
}

basic_results_for_one_function <- function(function.id, db) {
    functions <- db %>% tbl("functions") %>% rename(function_id = id, function_location = location, call_type = type)
    calls <- db %>% tbl("calls") %>% rename(call_id = id, call_location = location)
    arguments <- db %>% tbl("arguments") %>% rename(argument_id = id)
    promises <- db %>% tbl("promises") %>% rename(promise_id = id, promise_type = type)
    promise_associations <- db %>% tbl("promise_associations")
    promise_evaluations<- db %>% tbl("promise_evaluations") 
    

    function_info <- functions %>% filter(function_id == function.id)     
    
    function_info %>% 
        left_join(calls, by = "function_id") %>% 
        left_join(promise_associations, by = "call_id") %>%
        left_join(promises, by = "promise_id") %>%
        left_join(promise_evaluations, by = "promise_id")
}

basic_unagreggated_results_all_in_one_place <- function(path="trace.sqlite") {
    db <- load_trace(path)

    function_ids <- db %>% tbl("functions") %>% select(id) %>% rename(function_id = id)

    functions <- db %>% tbl("functions") %>% rename(function_id = id, function_location = location, call_type = type)
    calls <- db %>% tbl("calls") %>% rename(call_id = id, call_location = location)
    arguments <- db %>% tbl("arguments") %>% rename(argument_id = id)
    promises <- db %>% tbl("promises") %>% rename(promise_id = id, promise_type = type)
    promise_associations <- db %>% tbl("promise_associations")
    promise_evaluations<- db %>% tbl("promise_evaluations") 

    #function_info <- functions %>% filter(function_id == function.id)     
    
    functions %>% 
        left_join(calls, by = "function_id") %>% 
        left_join(promise_associations, by = "call_id") %>%
        left_join(promises, by = "promise_id") %>%
        left_join(promise_evaluations, by = "promise_id")
}

write_unagreggated_results <- function(results, file="trace.txt", ...) {
    printable <- 
        results %>% data.frame %>%
        mutate(definition = gsub("[\r\n]", "\\\\n", gsub("\t","\\\\t",definition))) %>% 
        select(
            function_name,
            function_id,
            call_id,
            argument_id,
            call_type,
            compiled,
            function_location,
            call_location,
            promise_id,
            parent_id,
            promise_type,
            original_type,
            clock,
            event_type,
            from_call_id,
            in_call_id,
            lifestyle,
            #pointer,
            definition)

    write.table(printable, file=file, sep="\t", row.names=FALSE, ...)   
}

humanize_promise_type = function(type, fallback_type=NULL)
    if(type == 0) "NIL" else
    if(type == 1) "SYM" else
    if(type == 2) "LIST" else
    if(type == 3) "CLOS" else
    if(type == 4) "ENV" else
    if(type == 5) "PROM" else
    if(type == 6) "LANG" else
    if(type == 7) "SPECIAL" else
    if(type == 8) "BUILTIN" else
    if(type == 9) "CHAR" else
    if(type == 10) "LGL" else
    if(type == 13) "INT" else
    if(type == 14) "REAL" else
    if(type == 15) "CPLX" else
    if(type == 16) "STR" else
    if(type == 17) "DOT" else
    if(type == 18) "ANY" else
    if(type == 19) "VEC" else
    if(type == 20) "EXPR" else
    if(type == 21) {
      if(is.null(fallback_type)) "BCODE"
      else paste("BCODE", Recall(fallback_type), sep=" ")
    } else
    if(type == 22) "EXTPTR" else
    if(type == 23) "WEAKREF" else
    if(type == 24) "RAW" else
    if(type == 25) "S4" else NULL

pretty_unagreggated_report <- function(path="trace.sqlite", file="trace.report.txt") {
    write("!", file);

    db <- load_trace(path)

    functions <- db %>% tbl("functions") %>% rename(function_id = id, function_location = location, call_type = type)
    calls <- db %>% tbl("calls") %>% rename(call_id = id, call_location = location)
    arguments <- db %>% tbl("arguments") %>% rename(argument_id = id, argument_name = name)
    promises <- db %>% tbl("promises") %>% rename(promise_id = id, promise_type = type)
    promise_associations <- db %>% tbl("promise_associations")
    promise_evaluations<- db %>% tbl("promise_evaluations") 

    promise_info <- promise_associations %>%
        left_join(promises, by = "promise_id") %>%
        left_join(promise_evaluations, by = "promise_id") %>%
        left_join(arguments, by = "argument_id") %>% 
        data.frame
   
    handle_function <- function(fun.row) {
        write(paste("FUNCTION:", fun.row$function_id), file, append=TRUE)
        write(paste("    location:", fun.row$location), file, append=TRUE)
        write(paste("    definition:\n        ", gsub("\n", "\n        ", fun.row$definition), sep=""), file, append=TRUE)
        write(file, append=TRUE)

        my.calls <- fun.row %>% left_join(calls, by = "function_id", copy=TRUE)        
        my.calls %>% group_by(call_id) %>% do(handle_call(.))

        write(file, append=TRUE)
        write(file, append=TRUE)
    }

    handle_call <- function(call.row) {
        args <- (arguments %>% filter(function_id == call.row$function_id) %>% arrange(position) %>% data.frame)$argument_name
        signature <- paste(c(call.row$function_name, " <- function(", paste(args, sep=","), ") ..."), sep="")
        type <- if (call.row$call_type == 0) "closure" else
                if (call.row$call_type == 1) "built-in" else
                if (call.row$call_type == 2) "special" else
                if (tcall.row$call_ype == 3) "primitive" else NULL


        write(paste("    CALL:", call.row$call_id), file, append=TRUE)
        write(paste(c("        signature:", signature)), file, append=TRUE)
        write(paste("        called by:", call.row$parent_id), file, append=TRUE)
        write(paste("        type:", type), file, append=TRUE)
        write(paste("        location:", call.row$call_location), file, append=TRUE)
        write(paste("        callsite:", "TODO"), file, append=TRUE)
        write(file)
       
        if (length(args) > 0) {
            my.promises <- call.row %>% left_join(promise_info, by = "call_id") %>% group_by(clock) %>% arrange(clock)
            my.promises %>% do(handle_promise(.))
        }

        data_frame()
    }

    handle_promise <- function(prom.row) {
        promise_type <- humanize_promise_type(prom.row$promise_type, prom.row$original_type)

        event_type <- if (prom.row$event_type == 15) "promise forced" else 
                     if(prom.row$event_type == 0) "promise lookup" else NULL

        lifestyle <- if(prom.row$lifestyle == 1) "local" else
                     if(prom.row$lifestyle == 2) "passed down" else
                     if(prom.row$lifestyle == 3) "escaped" else NULL

        write(paste("        PROMISE:", prom.row$argument_name, paste("(", prom.row$promise_id, ")", sep="")), file, append=TRUE)
        write(paste("            promise type:", promise_type), file, append=TRUE) 
        write(paste("            event:", event_type), file, append=TRUE)
        write(paste("            clock:", prom.row$clock), file, append=TRUE)
        write(paste("            created for:", prom.row$from_call_id), file, append=TRUE)
        write(paste("            evaluated in:", prom.row$in_call_id), file, append=TRUE)
        write(paste("            lifestyle:", lifestyle), file, append=TRUE)
        write(file, append=TRUE)
        
        data_frame()
    }

    functions %>% group_by(function_id) %>% do(handle_function(.))
}
