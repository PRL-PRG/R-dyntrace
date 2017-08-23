library(dplyr)
library(igraph)
library(hashmap)

if(!exists("path"))
  path <- "/home/kondziu/workspace/R-dyntrace/data/rivr.sqlite"

# pretty print
pp <- function(number) format(number, big.mark=",", scientific=FALSE, trim=FALSE, digits=2)
ppp <- function(number) paste(format(number, big.mark=",", scientific=FALSE, trim=FALSE, digits=2), "%", sep="")

db <- src_sqlite(path)

# tables
promises <- db %>% tbl("promises")
promise_evaluations <- db %>% tbl("promise_evaluations")
promise_associations <- db %>% tbl("promise_associations")
promise_returns <- db %>% tbl("promise_returns")
calls <- db %>% tbl("calls") %>% rename(call_id = id)
functions <- db %>% tbl("functions") %>% rename(function_id = id)
arguments <- db %>% tbl("arguments")
metadata <- db %>% tbl("metadata")

promise.forces <- promise_evaluations %>% filter(promise_id >= 0 && event_type == 15)
promise.lookups <- promise_evaluations %>% filter(promise_id >= 0 && event_type == 0)
alien.promise.forces <- promise_evaluations %>% filter(promise_id < 0 && event_type == 15)

n.functions <- (functions %>% count %>% data.frame)$n
n.calls <- (calls %>% count %>% data.frame)$n
n.promises <- (promises %>% count %>% data.frame)$n
n.alien.promises <- (promise_evaluations %>% filter(promise_id < 0) %>% group_by(promise_id) %>% count %>% data.frame)$promise_id %>% length
n.promise.forces <- (promise.forces %>% count %>% data.frame)$n
n.promise.lookups <- (promise.lookups %>% count %>% data.frame)$n 
n.alien.promise.forces <- (alien.promise.forces %>% count %>% data.frame)$n
n.alien.promise.lookups <- NA # I currently don't collect this information to save space

get_lifestyles <- function() {
  lifestyles <-
    left_join(promises, promise.forces, by=c("id" = "promise_id"))  %>%
    mutate(lifestyle = ifelse(is.na(lifestyle), "virgin",
                              ifelse(lifestyle == 1, "local",
                                     ifelse(lifestyle == 2, "branch-local",
                                            ifelse(lifestyle == 3, "escaped",
                                                   ifelse(lifestyle == 4, "immediate local",
                                                          ifelse(lifestyle == 5, "immediate branch-local",
                                                                 "???"))))))) %>%
    select(id, from_call_id, in_call_id, lifestyle, effective_distance_from_origin) %>%
    
    group_by(lifestyle) %>% count(lifestyle) %>%
    mutate(percent=((n*100/n.promise.forces))) %>% rename(number = n) 
  #group_by(lifestyle) #%>%
  #do(mutate(., percent = paste(format(percent, digits=12), "%", sep="") ))
  
  all.lifestyles <- 
    data.frame(lifestyle = c("immediate local", "local", "immediate branch-local", "branch-local", "escaped", "virgin")) %>% 
    left_join(lifestyles, by="lifestyle", copy=TRUE) %>% 
    mutate(number = ifelse(is.na(number), 0, number), 
           percent = ifelse(is.na(percent), 0, percent))
  
  all.lifestyles %>% arrange(desc(number))
}

get_effective_distances <- function(cutoff=NA) {
  effective.distances <-
    left_join(promises, promise.forces, by=c("id" = "promise_id"))  %>%
    select(id, from_call_id, in_call_id, lifestyle, effective_distance_from_origin) %>%
    
    group_by(effective_distance_from_origin) %>% count %>%
    mutate(percent=((n*100/n.promise.forces))) %>% rename(number = n)
  #group_by(lifestyle) #%>%
  #do(mutate(., percent = paste(format(percent, digits=12), "%", sep="") ))
  
  
  na.distance <- NA
  min.distance <- -1
  max.distance <- 
    (effective.distances %>% filter(!is.na(effective_distance_from_origin)) %>% data.frame)$effective_distance_from_origin %>% 
    max
  
  distance.range <- c(na.distance, min.distance:max.distance)
    
  if (!is.na(cutoff)) {
    if (max.distance > cutoff) 
      max.distance <- cutoff
   
    below <- 
      effective.distances %>% filter(effective_distance_from_origin <= cutoff) %>%
      collect %>% ungroup %>%
      mutate(effective_distance_from_origin = effective_distance_from_origin)
    
    above <- 
      effective.distances %>% filter(effective_distance_from_origin > cutoff) %>% 
      collect %>% ungroup %>% 
      summarise(
        effective_distance_from_origin=Inf, 
        number=sum(number), 
        percent=sum(percent))
    
    distance.range <- c(na.distance, min.distance:max.distance, Inf)
    
    effective.distances <- rbind(below, above)
  }
  
  histogram <- 
    data.frame(effective_distance_from_origin = distance.range) %>% 
    left_join(effective.distances, by="effective_distance_from_origin", copy=TRUE) %>% 
    rename(effective_distance = effective_distance_from_origin)
  
  histogram
  
}

get_actual_distances <- function(cutoff=NA) {
  actual.distances <-
    left_join(promises, promise.forces, by=c("id" = "promise_id"))  %>%
    select(id, from_call_id, in_call_id, lifestyle, actual_distance_from_origin) %>%
    
    group_by(actual_distance_from_origin) %>% count %>%
    mutate(percent=((n*100/n.promise.forces))) %>% rename(number = n)
  #group_by(lifestyle) #%>%
  #do(mutate(., percent = paste(format(percent, digits=12), "%", sep="") ))
  
  na.distance <- NA
  min.distance <- -1
  max.distance <- 
    (actual.distances %>% filter(!is.na(actual_distance_from_origin)) %>% data.frame)$actual_distance_from_origin %>% 
    max
  distance.range <- c(na.distance, min.distance:max.distance)
  
  if (!is.na(cutoff)) {
    if (max.distance > cutoff) 
      max.distance <- cutoff
    
    below <- 
      actual.distances %>% filter(actual_distance_from_origin <= cutoff) %>%
      collect %>% ungroup %>%
      mutate(actual_distance_from_origin = actual_distance_from_origin)
    
    above <- 
      actual.distances %>% filter(actual_distance_from_origin > cutoff) %>% 
      collect %>% ungroup %>% 
      summarise(
        actual_distance_from_origin=Inf, 
        number=sum(number), 
        percent=sum(percent))
    
    distance.range <- c(na.distance, min.distance:max.distance, Inf)
    
    actual.distances <- rbind(below, above)
  }
  
  histogram <- 
    data.frame(actual_distance_from_origin = distance.range) %>% 
    left_join(actual.distances, by="actual_distance_from_origin", copy=TRUE) %>% 
    rename(actual_distance = actual_distance_from_origin)
  
  histogram
}

get_promise_types <- function(cutoff=NA) {
  result <- 
    promises %>% 
    #mutate(archetype = type*100+if(is.na(original_type)) 99 else original_type) %>%
    group_by(type) %>% count(type) %>% 
    arrange(type) %>%
    mutate(percent=((n*100/n.promises))) %>%
    group_by(type) %>% 
    do(mutate(., 
              type_code = type, 
              type = humanize_promise_type(type)
              #percent = paste(format(percent, digits=12),   "%", sep="")
    )) %>%
    rename(number=n) %>%
    group_by(type) %>% select(type, number, percent) %>%
    data.frame %>%   
    arrange(desc(number))
  
  if (is.na(cutoff)) {
    result
  } else {
    above <- result %>% filter(percent >= cutoff)
    below <- result %>% filter(percent < cutoff) %>% summarise(type="other", number=sum(number), percent=sum(percent))  
    rbind(above, below)
  }
}

get_promise_return_types <- function(cutoff=NA) {
  result <- 
    promises %>% rename(promise_id=id) %>% select(promise_id) %>% left_join(promise_returns, by="promise_id") %>%
    group_by(type) %>% count(type) %>% 
    arrange(type) %>%
    mutate(percent=((n*100/n.promises))) %>%
    group_by(type) %>% 
    do(mutate(., 
              type_code = type, 
              type = humanize_promise_type(type)
              #percent = paste(format(percent, digits=12),   "%", sep="")
    )) %>%
    rename(number=n) %>%
    group_by(type) %>% select(type, number, percent) %>%
    data.frame %>%   
    arrange(desc(number))
  
  if (is.na(cutoff)) {
    result
  } else {
    above <- result %>% filter(percent >= cutoff)
    below <- result %>% filter(percent < cutoff) %>% summarise(type="other", number=sum(number), percent=sum(percent))  
    rbind(above, below)
  }
}

get_promise_return_types_by_type <- function(promise_type, cutoff=NA) {
  dehumanized_promise_type <- dehumanize_promise_type(promise_type)
  subset_of_promises <- promises %>% rename(promise_id=id) %>% filter(type==dehumanized_promise_type)
  number_of_promises_in_subset <- subset_of_promises %>% count %>% pull(n)
  result <- 
    subset_of_promises %>%
    select(promise_id) %>% left_join(promise_returns, by="promise_id") %>%
    group_by(type) %>% count(type) %>% 
    arrange(type) %>%
    mutate(percent=((n*100/n.promises))) %>%
    group_by(type) %>% 
    do(mutate(., 
              type_code = type, 
              type = humanize_promise_type(type)
              #percent = paste(format(percent, digits=12),   "%", sep="")
    )) %>%
    rename(number=n) %>%
    group_by(type) %>% select(type, number, percent) %>%
    data.frame %>%   
    arrange(desc(number))
  
  if (is.na(cutoff)) {
    result
  } else {
    above <- result %>% filter(percent >= cutoff)
    below <- result %>% filter(percent < cutoff) %>% summarise(type="other", number=sum(number), percent=sum(percent))  
    rbind(above, below)
  }
}

get_full_promise_types <- function(cutoff=NA) {
  result <-
    promises %>% 
    #mutate(archetype = type*100+if(is.na(original_type)) 99 else original_type) %>%
    group_by(full_type) %>% count(full_type) %>% 
    arrange(full_type) %>%
    mutate(percent=((n*100/n.promises))) %>%
    group_by(full_type) %>% 
    do(mutate(., 
              type = humanize_full_promise_type(full_type) 
              #percent = paste(format(percent, digits=12),   "%", sep="")
    )) %>%
    rename(number=n) %>%
    group_by(type) %>% select(type, number, percent) %>%
    data.frame %>%   
    arrange(desc(number))
  
  if (is.na(cutoff)) {
    result
  } else {
    above <- result %>% filter(percent >= cutoff)
    below <- result %>% filter(percent < cutoff) %>% summarise(type="other", number=sum(number), percent=sum(percent))  
    rbind(above, below)
  }
}

humanize_full_promise_type = function(full_type) {
  strsplit(full_type, ',', fixed=TRUE) %>% 
    sapply(., as.numeric) %>% 
    sapply(., function(type) humanize_promise_type(type)) %>% 
    paste(collapse = "→")
}

humanize_function_type = function(type) 
  ifelse(is.na(type), "NA",
    ifelse(type == 0, "closure",
      ifelse(type == 1, "built-in",
        ifelse(type == 2, "special",
          ifelse(type == 3, "primitive", NA)))))

dehumanize_function_type = function(type) 
  ifelse(is.na(type), "NA",
    ifelse(type == "closure", 0,
      ifelse(type == "built-in", 1,
        ifelse(type == "special", 2,
          ifelse(type == "primitive", 3, NA)))))

SEXP_TYPES <- hashmap(
  keys=c(0:10,13:25,69), 
  values=c(
    "NIL", "SYM", "LIST", "CLOS", "ENV",  "PROM", # 0-5
    "LANG", "SPECIAL", "BUILTIN", "CHAR",  "LGL", # 6-10
    "INT", "REAL", "CPLX", "STR", "DOT", "ANY",   # 13-18
    "VEC", "EXPR", "BCODE", "EXTPTR", "WEAKREF",  # 19-23
    "RAW", "S4", "..."))                          # 24-25, 69

SEXP_TYPES_REV <- hashmap(
  values=c(0:10,13:25,69), 
  keys=c(
    "NIL", "SYM", "LIST", "CLOS", "ENV",  "PROM", # 0-5
    "LANG", "SPECIAL", "BUILTIN", "CHAR",  "LGL", # 6-10
    "INT", "REAL", "CPLX", "STR", "DOT", "ANY",   # 13-18
    "VEC", "EXPR", "BCODE", "EXTPTR", "WEAKREF",  # 19-23
    "RAW", "S4", "..."))                          # 24-25, 69

humanize_promise_type = function(type) 
  ifelse(is.na(type), "NA", SEXP_TYPES[[type]])

dehumanize_promise_type = function(type) 
  ifelse(is.na(type), "NA", SEXP_TYPES_REV[[type]])

get_lookup_histogram <- function(cutoff=NA) {
  data <- promises %>% rename(promise_id = id) %>% left_join(promise.lookups, by="promise_id") %>% select(promise_id, event_type) %>% collect
  unevaluated <- data.frame(no.of.lookups=0, number=(data %>% filter(is.na(event_type)) %>% count %>% data.frame)$n)
  evaluated <- data %>% filter(!is.na(event_type)) %>% group_by(promise_id) %>% count %>% group_by(n) %>% count %>% rename(no.of.lookups=n, number=nn)
  new.data <- rbind(unevaluated, evaluated) 
  
  if (is.na(cutoff)) {
    new.data
  } else {
    above <- new.data %>% filter(no.of.lookups > cutoff) %>% ungroup %>% collect %>% summarise(no.of.lookups=Inf, number=sum(number))
    below <- new.data %>% filter(no.of.lookups <= cutoff)
    rbind(above, below)
  }
}

# FIXME what if empty
get_force_histogram <- function(cutoff=NA) {
  data <- promises %>% rename(promise_id = id) %>% left_join(promise.forces, by="promise_id") %>% select(promise_id, event_type) %>% collect
  unevaluated <- tibble(no.of.forces=0, number=(data %>% filter(is.na(event_type)) %>% count %>% data.frame)$n)
  evaluated <- data %>% filter(!is.na(event_type)) %>% group_by(promise_id) %>% count %>% group_by(n) %>% count %>% ungroup %>% rename(no.of.forces=n, number=nn) 
  
  new.data <- rbind(unevaluated, evaluated)
  
  if (is.na(cutoff)) {
    new.data %>% mutate(percent=((number*100/n.promises)))
  } else {
    above <- new.data %>% filter(no.of.forces > cutoff) %>% ungroup %>% collect %>% summarise(no.of.forces=Inf, number=sum(number))
    below <- new.data %>% filter(no.of.forces <= cutoff)
    rbind(above, below) %>% mutate(percent=((number*100/n.promises)))
  }
}

get_fuzzy_force_histogram <- function() {
  renamer = hashmap(0:4, c("forced more than once", "forced once and read", "only forced once", "never forced but read", "never forced, not read"))
  promises %>% rename(promise_id = id) %>% 
  left_join(promise_evaluations, by ="promise_id") %>% 
  group_by(promise_id) %>% 
  summarise(
    #evaluated = ifelse(is.na(event_type), 0, n()), 
    forced = ifelse(is.na(event_type), 0, sum(as.integer(event_type == 15))), 
    looked_up = ifelse(is.na(event_type), 0, sum(as.integer(event_type == 0)))) %>% 
  mutate(classification =
              ifelse((forced > 1),                    0,               # forced more than once
              ifelse((forced == 1 && looked_up > 0),  1,               # forced once and read
              ifelse((forced == 1 && looked_up ==0 ), 2,               # forced exactly once
              ifelse((forced == 0 && looked_up > 0),  3,               # not forced but read
              ifelse((forced == 0 && looked_up == 0), 4, NA)))))) %>%  # not forced, not read
  group_by(classification) %>% 
  summarise(number=n()) %>% as.data.frame %>%
  mutate(percent=(100*number/n.promises)) %>% 
  right_join(data.frame(classification=0:4), by="classification") %>%
  mutate(
    number=ifelse(is.na(number), 0, number),
    percent=ifelse(is.na(percent), 0, percent),
    classification = ifelse(is.na(classification), NA, renamer[[classification]]))
}

get_force_histogram_by_type <- function() {
  promise_types <- get_promise_types()
  promise_type_count <- hashmap(
    keys=promise_types$type,
    values=promise_types$number
  )
  
  data <- promises %>% rename(promise_id = id) %>% 
    left_join(promise.forces, by="promise_id") %>% 
    select(promise_id, type, full_type, event_type) %>% 
    collect
  
  unevaluated <- data %>% 
    filter(is.na(event_type)) %>% 
    group_by(type) %>% summarise(no.of.forces=as.integer(0), number=n()) %>% 
    as.data.frame
  
  evaluated <- data %>% 
    filter(!is.na(event_type)) %>% 
    group_by(promise_id) %>% summarise(no.of.forces=n(), type=c(type)) %>% 
    group_by(type, no.of.forces) %>% summarise(number=n()) %>% 
    as.data.frame
  
  intermediate <- 
    rbind(unevaluated, evaluated)
  
  histogram <- 
    merge( # cartesian product
      data.frame(type=intermediate$type %>% unique), 
      data.frame(no.of.forces=intermediate$no.of.forces %>% unique), 
      by=NULL) %>% 
    left_join(intermediate, by=c("type", "no.of.forces")) %>%
    mutate(number=ifelse(is.na(number), 0, number)) %>%
    arrange(type, no.of.forces) %>%
    mutate(type=humanize_promise_type(type)) %>%
    mutate(percent_within_type=((number*100/promise_type_count[[type]]))) %>%
    mutate(percent_overall=((number*100/n.promise.forces)))
  
  histogram
}

get_functions_by_type <- function() {
  functions %>% 
  group_by(type) %>% count %>% rename(number=n) %>% 
  collect %>% ungroup() %>%
  mutate(type=humanize_function_type(type), percent=100*number/n.functions)
}

get_calls_by_type <- function() {
  left_join(calls, select(functions, function_id, type), by="function_id") %>% 
    group_by(type) %>% count %>% rename(number=n) %>% 
    collect %>% ungroup() %>%
    mutate(type=humanize_function_type(type), percent=100*number/n.calls)
}

get_function_compilation_histogram <- function() {
  functions %>% 
    group_by(compiled) %>% count %>% rename(number=n) %>% 
    collect %>% ungroup() %>%
    mutate(compiled=as.logical(compiled), percent=100*number/n.functions)
}

get_call_compilation_histogram <- function() {
  left_join(calls, select(functions, function_id, type), by="function_id") %>% 
    group_by(compiled) %>% count %>% rename(number=n) %>% 
    collect %>% ungroup() %>%
    mutate(compiled=as.logical(compiled), percent=100*number/n.calls)
}

get_function_compilation_histogram_by_type <- function(specific_type=NA) {
  functions_by_type <- get_functions_by_type()
  functions_by_type_hashmap <- hashmap(functions_by_type$type, functions_by_type$number)
  
  specific_functions <- 
    if (is.na(specific_type)) {
      select(functions, function_id, type, compiled) 
    } else {
      dehumanized_type <- dehumanize_function_type(specific_type)
      select(functions, function_id, type, compiled) %>% 
        filter(type == dehumanized_type)
    }
  
  histogram <- specific_functions %>% 
    group_by(type, compiled) %>% count %>% rename(number=n) %>% 
    mutate(type=humanize_function_type(type), compiled=as.logical(compiled)) %>% 
    mutate(percent_overall=100*number/n.functions) %>%
    mutate(percent_within_type=100*number/functions_by_type_hashmap[[type]])
  
  if (is.na(specific_type))
    histogram
  else
    histogram %>% select(-type, -percent_overall)
}

get_call_compilation_histogram_by_type <- function(specific_type=NA) {
  calls_by_type <- get_calls_by_type()
  calls_by_type_hashmap <- hashmap(calls_by_type$type, calls_by_type$number)
  
  data <- 
    if (is.na(specific_type)) {
      left_join(calls, select(functions, function_id, type), by="function_id")
    } else {
      dehumanized_type <- dehumanize_function_type(specific_type)
      left_join(calls, select(functions, function_id, type), by="function_id") %>% 
        filter(type == dehumanized_type)
    }
  
  histogram <- data %>%
    group_by(type, compiled) %>% count %>% rename(number=n) %>% 
    collect %>% ungroup() %>%
    mutate(type=humanize_function_type(type)) %>% 
    mutate(compiled=ifelse(as.logical(compiled), "compiled", "uncompiled")) %>% 
    mutate(percent_overall=100*number/n.calls) %>%
    mutate(percent_within_type=100*number/calls_by_type_hashmap[[type]])
  
  if (is.na(specific_type))
    histogram
  else
    histogram %>% select(-type, -percent_overall)
}

# This one checks in the calls rather than in the function, so functions which get compiled on the fly will register as such.
get_function_compilation_histogram_by_type_actual <- function(specific_type=NA) {
  functions_by_type <- get_functions_by_type()
  functions_by_type_hashmap <- hashmap(functions_by_type$type, functions_by_type$number)

  specific_functions <- 
    if (is.na(specific_type)) {
      select(functions, function_id, type) 
    } else {
      dehumanized_type <- dehumanize_function_type(specific_type)
      select(functions, function_id, type) %>% filter(type == dehumanized_type)
    }
  
  histogram <- 
    left_join(specific_functions, select(calls, call_id, function_id, compiled), by="function_id") %>% 
    group_by(function_id) %>% summarise(runs=count(), compiled_runs=sum(compiled), type=type) %>% 
    mutate(compiled=ifelse(compiled_runs == 0, 0, as.character(ifelse(runs == compiled_runs, 1, ifelse(compiled_runs == runs - 1, 2, 3))))) %>% 
    group_by(type, compiled) %>% count %>% rename(number=n) %>%
    collect %>% ungroup() %>%
    mutate(compiled=ifelse(compiled == 1, "compiled", ifelse(compiled == 2, "after 1st", ifelse(compiled == 0, "uncompiled", "erratic")))) %>%
    mutate(type=humanize_function_type(type)) %>%
    mutate(percent_overall=100*number/n.functions) %>%
    mutate(percent_within_type=100*number/functions_by_type_hashmap[[type]])
    
  if (is.na(specific_type))
    histogram
  else
    histogram %>% select(-type, -percent_overall)
}

get_promise_evaluation_histogram <- function(cutoff=NA) {
  data <- 
    promises %>% rename(promise_id = id) %>% 
    left_join(promise_evaluations, by="promise_id") %>% 
    collect
  
  unevaluated <- tibble(
    no.of.evaluations=0, 
    number=(data %>% filter(is.na(event_type)) %>% count %>% data.frame)$n)
  
  evaluated <- 
    data %>% filter(!is.na(event_type)) %>% 
    group_by(promise_id) %>% count %>% 
    group_by(n) %>% count %>% 
    ungroup %>% rename(no.of.evaluations=n, number=nn)
  
  new.data <- rbind(unevaluated, evaluated) 
  
  if (is.na(cutoff)) {
    new.data %>% mutate(percent=(number*100/n.promises))
  } else {
    above <- new.data %>% filter(no.of.evaluations > cutoff) %>% ungroup %>% collect %>% summarise(no.of.evaluations=Inf, number=sum(number))
    below <- new.data %>% filter(no.of.evaluations <= cutoff)
    rbind(below, above) %>% mutate(percent=(number*100/n.promises))
  }
}

get_function_calls <- function(...) {
  patterns <- list(...)
  data <- 
    left_join(functions, calls, by="function_id") %>% 
    select(function_name, function_id) %>% 
    #distinct(function_name, function_id) %>% 
    group_by(function_name, function_id) %>% count() %>% rename(number = n) %>%
    mutate(percent = ((number * 100) / n.calls)) %>%
    mutate(function_name = ifelse(is.na(function_name), "<anonymous>", function_name)) %>%
    select(function_id, function_name, number, percent) %>%
    collect(n=Inf)
    lapply(patterns, function(pattern) {filter(data, grepl(pattern, function_name))} ) %>% bind_rows
}

get_call_tree <- function() {
  data <- 
    calls %>% 
    arrange(call_id) %>% 
    select(call_id, parent_id, function_id) %>%
    as.data.frame
  
  edges <- c(rbind(data$parent_id, data$call_id))
  nodes <- unique(edges)
  function_ids <- (data.frame(call_id=nodes) %>% left_join(data, by="call_id"))$function_id
  
  G <- 
    graph.empty(n = 0, directed = T) %>%
    add.vertices(length(nodes), attr = list(name = as.character(nodes)), function_id = as.character(function_ids)) %>%
    add.edges(as.character(edges))
  
  G
}

traverse_call_tree <- function() {
  calls_df <- calls %>% 
    left_join(functions %>% select(function_id, type), by="function_id") %>% 
    select(call_id, parent_id, function_id, type) %>% 
    data.frame

  original_node_vector <- calls_df$call_id
  original_function_vector <- calls_df$function_id
  original_function_type_vector <- calls_df$type
    
  function_dict <- hashmap(keys=original_node_vector, values=original_function_vector)
  parent_dict <- hashmap(keys=original_node_vector, values=calls_df$parent_id)
  
  traverse <- function(cursor_node_vector, 
                       cursor_function_vector, 
                       repetitions = rep(0, length(cursor_node_vector)), 
                       height = rep(0, length(cursor_node_vector))) {
    
    parent_node_vector <- parent_dict[[cursor_node_vector]]
    parent_function_vector <- function_dict[[parent_node_vector]]

    if(all(is.na(parent_node_vector)))
      data.frame(
        call_id=original_node_vector, 
        function_id=original_function_vector, 
        type=humanize_function_type(original_function_type_vector), 
        recursive=repetitions>0,
        repetitions, 
        height)
    else            
      Recall(
        parent_node_vector, 
        parent_function_vector, 
        repetitions + ifelse(is.na(parent_function_vector), 0, (parent_function_vector == original_function_vector)), 
        height + as.integer(!is.na(parent_node_vector)))
  }
  
  traverse(original_node_vector, original_function_vector)
}

get_recursion_info_by_function <- function(traverse_data) {
  traverse_data %>% 
    group_by(function_id) %>% 
    summarize(
      mean_repetitions=mean(repetitions), 
      min_repetitions=min(repetitions), 
      max_repetitions=max(repetitions), 
      mean_height=mean(height), 
      min_height=min(height), 
      max_height=max(height), 
      number=n(), 
      number_repeating=sum(as.integer(recursive)),
      percent_repeating=100*number_repeating/number) %>%
    as.data.frame
}

get_call_recursion_histogram <- function(traverse_data) {
  traverse_data %>% 
    group_by(recursive) %>% 
    summarize(
      number=n(), 
      percent=100*number/n.calls) %>%
    as.data.frame
}

get_function_recursion_histogram <- function(traverse_data) {
  classification_table <- c( "0",
    "(0,10〉", "(10,20〉", "(20,30〉", 
    "(30,40〉", "(40,50〉", "(50,60〉", 
    "(60,70〉", "(70,80〉", "(80,90〉", 
    "(90,100〉")
  
  classify <- function(percentage)
    ifelse(percentage == 0, 
           classification_table[1], 
           classification_table[((percentage - 1) %/% 10 + 2)])

  histogram <- 
    traverse_data %>% 
    group_by(function_id) %>% 
    summarize(
      number=n(), 
      number_repeating=sum(as.integer(recursive)),
      percent_repeating=100*number_repeating/number) %>%
    mutate(recursion_rate = classify(percent_repeating)) %>%
    group_by(recursion_rate) %>% 
    summarize(
      number=n(), 
      percent=100*n()/n.functions) %>%
    as.data.frame
  
  data.frame(recursion_rate=classification_table) %>% 
    left_join(histogram, by="recursion_rate") %>% 
    mutate(
      number=ifelse(is.na(number), 0, number), 
      percent=ifelse(is.na(percent), 0, percent))
  
}

get_call_strictness <- function() {
  histogram <- 
    calls %>% left_join(promise_associations, by="call_id") %>% 
    filter(!is.na(promise_id)) %>%
    left_join(promise.forces, by="promise_id") %>% 
    group_by(call_id) %>% summarise(
      unevaluated=sum(as.integer(is.na(event_type))), 
      escaped=sum(as.integer(!is.na(event_type) && (lifestyle == 3))), 
      evaluated=sum(as.integer(!is.na(event_type) && (lifestyle != 3))), 
      count=n()) %>%
    collect %>% ungroup() %>%
    mutate(strict=(evaluated==count)) %>%
    group_by(strict) %>% 
    summarise(number=n(), percent=100*n()/n.calls)
  
  nas <-
    calls %>% left_join(promise_associations, by="call_id") %>% 
    filter(is.na(promise_id)) %>% 
    count() %>% rename(number=n) %>%
    mutate(strict=NA, percent=100*number/n.calls) %>%
    collect
  
  rbind(histogram, nas)
}

get_call_strictness_ratios <- function() {
  histogram <- 
    calls %>% 
      left_join(promise_associations, by="call_id") %>% 
      filter(!is.na(promise_id)) %>%
      left_join(promise.forces, by="promise_id") %>% 
      group_by(call_id) %>% summarise(
        unevaluated=sum(as.integer(is.na(event_type))), 
        escaped=sum(as.integer(!is.na(event_type) && (lifestyle == 3))), 
        evaluated=sum(as.integer(!is.na(event_type) && (lifestyle != 3))), 
        count=n()) %>%
      collect %>% ungroup %>%
      mutate(strictness_ratio=paste(evaluated, count, sep="/")) %>%
      rename(promises=count) %>%
      group_by(strictness_ratio, promises, evaluated) %>% 
      summarise(number=n(), percent=100*n()/n.calls)
  
  nas <-
    calls %>% left_join(promise_associations, by="call_id") %>% 
    filter(is.na(promise_id)) %>% 
    count() %>% rename(number=n) %>%
    collect %>% ungroup() %>%
    mutate(strictness_ratio="0/0", percent=100*number/n.calls, evaluated=0, promises=0)
  
  rbind(nas %>% as.data.frame, histogram %>% as.data.frame) %>% 
    arrange(promises, evaluated) %>% 
    select(strictness_ratio, number, percent, -promises, -evaluated) 
}

get_call_strictness_rate <- function() {
  classification_table <- c( "0", 
    "(0,10〉", "(10,20〉", "(20,30〉", 
    "(30,40〉", "(40,50〉", "(50,60〉", 
    "(60,70〉", "(70,80〉", "(80,90〉", 
    "(90,100)", "100")
  
  classify <- function(percentage)
    ifelse(percentage == 0, 
           classification_table[1], 
           ifelse(percentage == 100, 
                  classification_table[12],        
                  classification_table[((percentage - 1) %/% 10 + 2)]))
  
  histogram <- 
    calls %>% 
    left_join(promise_associations, by="call_id") %>% 
    filter(!is.na(promise_id)) %>%
    left_join(promise.forces, by="promise_id") %>% 
    group_by(call_id) %>% 
    summarise(
      #unevaluated=sum(as.integer(is.na(event_type))), 
      #escaped=sum(as.integer(!is.na(event_type) && (lifestyle == 3))), 
      evaluated=sum(as.integer(!is.na(event_type) && (lifestyle != 3))), 
      count=n()) %>%
    collect %>% ungroup() %>%
    mutate(
      strictness_rate_percent=(100*evaluated/count),
      strictness_rate=classify(strictness_rate_percent)) %>%
    group_by(strictness_rate) %>% 
    summarise(number=n(), percent=100*n()/n.calls)
  
  complete_histogram <-
    data.frame(strictness_rate=classification_table) %>% 
    left_join(histogram, by="strictness_rate") %>% 
    collect %>% ungroup() %>%
      mutate(
        number=ifelse(is.na(number), 0, number), 
        percent=ifelse(is.na(percent), 0, percent))
  
  nas <-
    calls %>% left_join(promise_associations, by="call_id") %>% 
    filter(is.na(promise_id)) %>% 
    count() %>% rename(number=n) %>%
    collect %>% ungroup() %>%
    mutate(strictness_rate=NA, percent=100*number/n.calls)
  
  rbind(complete_histogram, nas)
}

get_function_strictness <- function() {
  histogram <- 
    calls %>% 
    left_join(functions, by="function_id") %>% 
    left_join(promise_associations, by="call_id") %>% 
    filter(!is.na(promise_id)) %>%
    left_join(promise.forces, by="promise_id") %>% 
    group_by(call_id, function_id) %>% 
    summarise(
      #unevaluated=sum(as.integer(is.na(event_type))), 
      #escaped=sum(as.integer(!is.na(event_type) && (lifestyle == 3))), 
      evaluated=sum(as.integer(!is.na(event_type) && (lifestyle != 3))), 
      count=n()) %>%
    collect %>% ungroup() %>%
    mutate(strict=(evaluated==count)) %>%
    group_by(function_id, strict) %>% summarise() %>%
    group_by(strict) %>%
    summarise(
      number=n(), 
      percent=100*n()/n.functions)

  nas <-
    calls %>% 
    left_join(promise_associations, by="call_id") %>% 
    left_join(functions, by="function_id") %>% 
    filter(is.na(promise_id)) %>% 
    group_by(function_id) %>%
    summarise() %>% 
    count() %>% rename(number=n) %>%
    collect %>% ungroup() %>%
    mutate(strict=NA, percent=100*number/n.functions)
  
  complete_histogram <- 
       rbind(histogram %>% data.frame, nas %>% data.frame) 
  
  complete_histogram
}

get_function_strictness_rate <- function() {
    classification_table <- c( "0", 
    "(0,10〉", "(10,20〉", "(20,30〉", 
    "(30,40〉", "(40,50〉", "(50,60〉", 
    "(60,70〉", "(70,80〉", "(80,90〉", 
    "(90,100)", "100")
  
  classify <- function(percentage)
    ifelse(percentage == 0, 
           classification_table[1], 
           ifelse(percentage == 100, 
                classification_table[12],        
                classification_table[((percentage - 1) %/% 10 + 2)]))
  
  histogram <- 
    calls %>% 
    left_join(promise_associations, by="call_id") %>% 
    filter(!is.na(promise_id)) %>%
    left_join(promise.forces, by="promise_id") %>% 
    group_by(call_id, function_id) %>% 
    summarise(
      #unevaluated=sum(as.integer(is.na(event_type))), 
      #escaped=sum(as.integer(!is.na(event_type) && (lifestyle == 3))), 
      evaluated=sum(as.integer(!is.na(event_type) && (lifestyle != 3))), 
      count=n()) %>%
    mutate(strict=(evaluated==count)) %>%
    group_by(function_id) %>%
    summarise(
      strict_calls=sum(as.integer(strict)),
      nonstrict_calls=sum(as.integer(!strict)),
      count=n()) %>%
    collect %>% ungroup %>%
    mutate(strict=strict_calls==count) %>%
    mutate(percentage=(100*strict_calls/count)) %>%
    mutate(strictness_rate=classify(percentage)) %>%
    group_by(strictness_rate) %>%
    summarise(
      number=n(), 
      percent=100*n()/n.functions) %>%
    collect
  
  complete_histogram <-
    data.frame(strictness_rate=classification_table) %>% 
    left_join(histogram, by="strictness_rate") %>% 
      mutate(
        number=ifelse(is.na(number), 0, number), 
        percent=ifelse(is.na(percent), 0, percent)) %>%
    collect
  
  nas <-
    calls %>% left_join(promise_associations, by="call_id") %>% 
    filter(is.na(promise_id)) %>% 
    group_by(function_id) %>%
    summarise() %>% count() %>% rename(number=n) %>%
    mutate(strictness_rate=NA, percent=100*number/n.functions) %>%
    collect
  
  rbind(complete_histogram, nas)
}

get_function_strictness_rate <- function() {
    classification_table <- c( "0", 
    "(0,10〉", "(10,20〉", "(20,30〉", 
    "(30,40〉", "(40,50〉", "(50,60〉", 
    "(60,70〉", "(70,80〉", "(80,90〉", 
    "(90,100)", "100")
  
  classify <- function(percentage)
    ifelse(percentage == 0, 
           classification_table[1], 
           ifelse(percentage == 100, 
                classification_table[12],        
                classification_table[((percentage - 1) %/% 10 + 2)]))
  
  histogram <- 
    calls %>% 
    left_join(promise_associations, by="call_id") %>% 
    filter(!is.na(promise_id)) %>%
    left_join(promise.forces, by="promise_id") %>% 
    group_by(call_id, function_id) %>% 
    summarise(
      #unevaluated=sum(as.integer(is.na(event_type))), 
      #escaped=sum(as.integer(!is.na(event_type) && (lifestyle == 3))), 
      evaluated=sum(as.integer(!is.na(event_type) && (lifestyle != 3))), 
      count=n()) %>%
    collect %>% 
    mutate(strict=(evaluated==count)) %>%
    group_by(function_id) %>%
    summarise(
      strict_calls=sum(as.integer(strict)),
      nonstrict_calls=sum(as.integer(!strict)),
      count=length(strict)) %>%
    mutate(strict=strict_calls==count) %>%
    mutate(percentage=100*strict_calls/count) %>%
    mutate(strictness_rate=classify(percentage)) %>%
    group_by(strictness_rate) %>%
    summarise(
      number=n(), 
      percent=100*n()/n.functions)
  
  complete_histogram <-
    data.frame(strictness_rate=classification_table) %>% 
    left_join(histogram, by="strictness_rate") %>% 
      transform(
        number=ifelse(is.na(number), 0, number), 
        percent=ifelse(is.na(percent), 0, percent))
  
  nas <-
    calls %>% left_join(promise_associations, by="call_id") %>% 
    filter(is.na(promise_id)) %>% 
    group_by(function_id) %>%
    summarise() %>% count() %>% rename(number=n) %>%
    transform(strictness_rate=NA, percent=100*number/n.functions)
  
  rbind(complete_histogram, nas)
}

make_labels = function(x) ifelse(is.na(x), "NA", x)

get_call_strictness_by_type <- function() {
  histogram <- 
    calls %>% left_join(functions, by="function_id") %>% 
    left_join(promise_associations, by="call_id") %>% 
    filter(!is.na(promise_id)) %>%
    left_join(promise.forces, by="promise_id") %>% 
    group_by(call_id,type) %>% summarise(
      unevaluated=sum(as.integer(is.na(event_type))), 
      escaped=sum(as.integer(!is.na(event_type) && (lifestyle == 3))), 
      evaluated=sum(as.integer(!is.na(event_type) && (lifestyle != 3))), 
      count=n()) %>%
    mutate(strict=(evaluated==count)) %>%
    group_by(strict,type) %>% 
    summarise(number=n()) %>%
    mutate(percent=100*number/n.calls) %>%
    collect
  
  nas <-
    calls %>% left_join(functions, by="function_id") %>% 
    left_join(promise_associations, by="call_id") %>% 
    filter(is.na(promise_id)) %>% 
    group_by(type) %>% count() %>% rename(number=n) %>%
    mutate(strict=NA) %>%
    mutate(percent=100*number/n.calls) %>%
    collect
  
  intermediate <- 
       rbind(histogram %>% data.frame, nas %>% data.frame)
  
  complete_histogram <-
    merge( # cartesian product
       data.frame(type=intermediate$type %>% unique), 
       data.frame(strict=intermediate$strict %>% unique), 
       by=NULL) %>%
    left_join(intermediate, by=c("type", "strict")) %>%
    mutate(
      number=ifelse(is.na(number), 0, number),
      percent=ifelse(is.na(percent), 0, percent)) %>%
    mutate(type=humanize_function_type(type))
  
  complete_histogram
}

get_function_strictness_by_type <- function() {
  histogram <- 
    calls %>% 
    left_join(functions, by="function_id") %>% 
    left_join(promise_associations, by="call_id") %>% 
    filter(!is.na(promise_id)) %>%
    left_join(promise.forces, by="promise_id") %>% 
    group_by(call_id, function_id, type) %>% 
    summarise(
      unevaluated=sum(as.integer(is.na(event_type))), 
      escaped=sum(as.integer(!is.na(event_type) && (lifestyle == 3))), 
      evaluated=sum(as.integer(!is.na(event_type) && (lifestyle != 3))), 
      count=n()) %>%
    collect %>% ungroup %>%
    mutate(strict=(evaluated==count)) %>%
    group_by(function_id, type, strict) %>% summarise() %>%
    group_by(strict, type) %>%
    summarise(
      number=n(), 
      percent=100*n()/n.functions)

  nas <-
    calls %>% 
    left_join(promise_associations, by="call_id") %>% 
    left_join(functions, by="function_id") %>% 
    filter(is.na(promise_id)) %>% 
    group_by(function_id,type) %>%
    summarise() %>% 
    group_by(type) %>%
    count() %>% rename(number=n) %>%
    ungroup %>% collect %>%
    mutate(strict=NA, percent=100*number/n.functions) 
  
  intermediate <- 
       rbind(histogram %>% data.frame, nas %>% data.frame)
  
  complete_histogram <-
    merge( # cartesian product
       data.frame(type=intermediate$type %>% unique), 
       data.frame(strict=intermediate$strict %>% unique), 
       by=NULL) %>%
    left_join(intermediate, by=c("type", "strict")) %>%
    mutate(
      number=ifelse(is.na(number), 0, number),
      percent=ifelse(is.na(percent), 0, percent)) %>%
    mutate(type=humanize_function_type(type))
  
  complete_histogram
}

# i filter out call_id == 0
get_call_promise_evaluation_order <- function() {
  data <- calls %>% #filter(call_id==4) %>%
    left_join(promise_associations, by="call_id") %>% 
    left_join(promises %>% rename(promise_id = id), by="promise_id") %>% 
    left_join(arguments %>% rename(argument_id=id), by=c("argument_id", "call_id")) %>% 
    left_join(promise_evaluations, by="promise_id") %>%
    select(call_id, promise_id, argument_id, function_id, from_call_id, clock, event_type, name, position) %>%
    collect %>%
    arrange(clock)
  
  # strictness_signatures <- data %>% 
  #   filter(!is.na(argument_id)) %>%
  #   mutate(
  #     # symbol=paste("⟦", name, "⟧",
  #     #              ifelse(is.na(event_type), "∅", 
  #     #              ifelse(event_type == 15, "!", 
  #     #              ifelse(event_type == 0, "=", "#"))), 
  #     #              sep=""),
  #     code = paste(position,
  #                  ifelse(is.na(event_type), "∅", 
  #                  ifelse(event_type == 15, "!", 
  #                  ifelse(event_type == 0, "=", "#"))), 
  #                  sep="")) %>%
  #   group_by(call_id) %>%
  #   summarise(
  #     # strictness_signature = paste(symbol, collapse="→"),
  #     evaluation_order = paste(code, collapse="→"))
  
  force_signatures <- data %>%
    filter(event_type == 15) %>% 
    group_by(call_id) %>%
    summarise(
      #force_signature = paste("⟦", name, "⟧", sep="", collapse="→"),
      force_order = paste(position, collapse="→"))
  
  calls %>% select(call_id) %>% collect %>% 
    # left_join(strictness_signatures, by = "call_id") %>% 
    left_join(force_signatures, by = "call_id") #%>%
    #mutate(
      #strictness_signature=ifelse(is.na(strictness_signature), "", strictness_signature),
      #evaluation_order=ifelse(is.na(evaluation_order), "", evaluation_order),
      ##force_signature=ifelse(is.na(force_signature), "", force_signature),
      #force_order=ifelse(is.na(force_order), "", force_order))
}

get_function_promise_evaluation_order <- function(call_promise_evaluation_order) {
  function_list <- functions %>% select(function_id) %>% collect# %>% 
  unique_signatures =  
    function_list %>%
    left_join(call_promise_evaluation_order %>% 
              left_join(calls %>% 
                        select(call_id, function_id) %>% 
                        collect, 
                        by="call_id"), 
              by="function_id") %>% 
    filter(!is.na(force_order)) %>%
    group_by(function_id) %>% 
    summarise(
      # strictness_signature=length(unique(strictness_signature)), 
      # evaluation_order=length(unique(evaluation_order)), 
      force_orders=length(unique(force_order)),
      calls=n()) %>%
    right_join(function_list, by ="function_id")
  
  unique_signatures %>% collect
}

# FIXME returns NA for calls and percent.calls for no.of.force.orders==0
get_function_promise_force_order_histogram <- function(function_promise_evaluation_order, cutoff=NA) {
  data <- 
    function_promise_evaluation_order %>% 
    group_by(force_orders) %>% 
    summarise(
      number=n(),
      calls=sum(calls))%>% 
    rename(no.of.force.orders=force_orders) %>% 
    mutate(
      percent=(number*100/n.functions),
      percent.calls=(calls*100/n.calls))
  
  nas <- data %>% filter(is.na(no.of.force.orders)) %>% ungroup %>% mutate(no.of.force.orders = 0)
  new <- data %>% filter(!is.na(no.of.force.orders)) %>% arrange(no.of.force.orders)
    
  if (is.na(cutoff) || max(data$no.of.force.orders, na.rm=TRUE) <= cutoff) {
    rbind(nas %>% as.data.frame, new %>% as.data.frame)
  } else {
    above <- new %>% filter(no.of.force.orders > cutoff) %>% ungroup %>% collect %>% summarise(no.of.force.orders=Inf, number=sum(number), calls=sum(calls), percent=(number*100/n.functions), percent.calls=(calls*100/n.calls))
    below <- new %>% filter(no.of.force.orders <= cutoff)
    rbind(nas %>% as.data.frame, below %>% as.data.frame, above %>% as.data.frame) 
  }
}

get_strict_function_promise_force_order_histogram <- function(function_promise_evaluation_order, cutoff=NA) {
  strict_functions %>%
    calls %>% 
    left_join(promise_associations, by="call_id") %>% 
    filter(!is.na(promise_id)) %>%
    left_join(promise.forces, by="promise_id") %>% 
    group_by(call_id, function_id) %>% 
    summarise(
      evaluated=sum(as.integer(!is.na(event_type) && (lifestyle != 3))), 
      count=n()) %>%
    mutate(strict=(evaluated==count)) %>%
    group_by(function_id) %>% 
    summarise(
      strict_calls=sum(as.integer(strict)),
      nonstrict_calls=sum(as.integer(!strict)),
      count=length(strict))
    select(call_id) 
  
  data <- 
    function_promise_evaluation_order %>% 
    group_by(force_order) %>% count %>% 
    rename(no.of.force.orders=force_order, number=n) %>% 
    mutate(percent=(number*100/n.functions))
  
  nas <- data %>% filter(is.na(no.of.force.orders)) %>% ungroup %>% mutate(no.of.force.orders = 0)
  new <- data %>% filter(!is.na(no.of.force.orders)) %>% arrange(no.of.force.orders)
    
  if (is.na(cutoff) || max(data$no.of.force.orders, na.rm=TRUE) <= cutoff) {
    rbind(nas %>% as.data.frame, new %>% as.data.frame)
  } else {
    above <- new %>% filter(no.of.force.orders > cutoff) %>% ungroup %>% collect %>% summarise(no.of.force.orders=Inf, number=sum(number), percent=(number*100/n.functions))
    below <- new %>% filter(no.of.force.orders <= cutoff)
    rbind(nas %>% as.data.frame, below %>% as.data.frame, above %>% as.data.frame) 
  }
}

## TODO : promise IDs are wrong! 
##        the 2nd, 3rd etc. vignettes don't 
# get_free_promise_evaluation_histogram <- function() {
#   promise.forces %>% 
#     mutate(free=in_prom_id==0) %>% 
#     group_by(free) %>% count() %>% rename(number=n) %>% 
#     as.data.frame %>%
#     mutate(free=as.logical(free), percent=100*number/n.promise.forces)
# }

# get_free_promise_evaluation_histogram <- function() {
#   promise.forces %>% 
#     mutate(free=in_prom_id==0) %>% 
#     group_by(free) %>% count() %>% rename(number=n) %>% 
#     as.data.frame %>%
#     mutate(free=as.logical(free), percent=100*number/n.promise.forces)
# }

# get_free_promise_evaluation_histogram <- function() {
#   promise.forces %>% 
#     mutate(free=promise_stack_depth < 2) %>% 
#     group_by(free) %>% count %>% rename(number=n) %>%
#     collect %>% ungroup %>%
#     mutate(free=as.logical(free), percent=100*number/n.promise.forces) %>%
#     as.data.frame 
# }

# assumes one promise ==> one or zero forces
get_promises_forced_by_another_evaluation_histogram <- function() {
  promises %>% 
    rename(promise_id=id) %>% rename(created_in=in_prom_id) %>% select(promise_id, created_in) %>% 
    left_join(promise.forces, by="promise_id") %>% 
    rename(forced_in=in_prom_id) %>% select(promise_id, created_in, forced_in) %>% 
    mutate(forced_by_another=created_in!=forced_in) %>% 
    group_by(forced_by_another) %>% summarise(number=n()) %>% ungroup %>% 
    collect %>% 
    mutate(forced_by_another=as.logical(forced_by_another), percent=(100*number/n.promises))
}

get_cascading_promises_histogram <- function (cutoff=NA) {
  basic <- promises %>% 
    rename(promise_id=id) %>% rename(created_in=in_prom_id) %>% select(promise_id, created_in) %>% 
    left_join(promise.forces, by="promise_id") %>% 
    rename(forced_in=in_prom_id) %>% 
    mutate(forced_by_another=created_in!=forced_in)
    
  nas <- 
    basic %>%
    filter(is.na(forced_by_another)) %>%
    count %>%
    rename (number=n) %>%
    mutate(number_of_forced_promises=NA) %>%
    select (number_of_forced_promises, number) %>%
    data.frame
  
  zero <- 
    basic %>% 
    filter(!is.na(forced_by_another)) %>%
    filter(!forced_by_another)%>%
    count %>%
    rename (number=n) %>%
    mutate(number_of_forced_promises=0) %>%
    select (number_of_forced_promises, number) %>%
    data.frame
  
  forcing <-
    basic %>%
    filter(!is.na(forced_by_another)) %>%
    filter(forced_by_another)%>%
    group_by(forced_in) %>% count %>% rename(number_of_forced_promises=n) %>% 
    group_by(number_of_forced_promises) %>% count %>% rename(number=n) %>%
    data.frame
  
  histogram <- rbind(nas, zero, forcing) %>% mutate(percent=100*number/sum(number))
  
  if (is.na(cutoff)) {
     histogram
  } else {
    above <- histogram %>% filter(number_of_forced_promises > cutoff) %>% ungroup %>% summarise(number_of_forced_promises=Inf, number=sum(number), percent=sum(percent))
    below <- histogram %>%  filter(number_of_forced_promises <= cutoff)
    rbind(below, above)
  }
}


# get_cascading_promises_evaluation_histogram <- function(cutoff=NA, include.toplevel=TRUE) {
#   basic <- 
#     promise.forces %>% 
#     group_by(in_prom_id) %>% count() %>% rename(no.of.forced.promises.inside=n) %>% 
#     collect
# 
#   grouped.real <- 
#     basic %>% 
#     mutate(toplevel=in_prom_id == 0) %>% 
#     group_by(toplevel, no.of.forced.promises.inside) %>% count() %>% rename(number=n) %>% 
#     as.data.frame
#    
#   grouped.zeros <- 
#     data.frame(toplevel=FALSE, 
#                no.of.forced.promises.inside=0, 
#                number=n.promise.forces-sum(grouped.real$number))
#   
#   full <- rbind(grouped.zeros, grouped.real)
#   
#   if (!include.toplevel) {
#     full <- full %>% filter(toplevel==FALSE) %>% select(-toplevel)
#     total <- sum(full$number)
#     full <- full %>% mutate(percent=100*number/total)
#   } else {
#     full <- full %>% mutate(percent=100*number/n.promise.forces)
#   }
# 
#   if (is.na(cutoff)) {
#     full
#   } else {
#     above <- 
#       if(include.toplevel)
#         full %>% 
#         filter(no.of.forced.promises.inside > cutoff) %>% 
#         ungroup %>% collect %>% group_by(toplevel) %>%
#         summarise(no.of.forced.promises.inside=Inf, 
#                   number=sum(number), 
#                   percent=sum(percent))
#       else
#         full %>% 
#         filter(no.of.forced.promises.inside > cutoff) %>% 
#         ungroup %>% collect %>% 
#         summarise(no.of.forced.promises.inside=Inf, 
#                   number=sum(number), 
#                   percent=sum(percent))
#     
#     below <- 
#       full %>% 
#       filter(no.of.forced.promises.inside <= cutoff)
#     rbind(below, above)
#   }
# }



# TODO
# order how many calls in those functions
# look at strict and non-strict arguments, 
# evaluation order but argument positions
# promises created for default evaluated
# promise evaluates to what?? (and by type)
# log scale
# heuristics for argument evaluation/strictness