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
n.alien.promise.lookups <- NA # I currently don't collect this information to save

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

humanize_promise_type = function(type) # TODO: kill
  if(is.na(type)) "NA" else
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
                                          if(type == 21) "BCODE" else
                                            if(type == 22) "EXTPTR" else
                                              if(type == 23) "WEAKREF" else
                                                if(type == 24) "RAW" else
                                                  if(type == 25) "S4" else 
                                                    if(type == 69) "..." else NA

SEXP_TYPES <- hashmap(
  keys=c(0:10,13:25,69), 
  values=c(
    "NIL", "SYM", "LIST", "CLOS", "ENV",  "PROM", # 0-5
    "LANG", "SPECIAL", "BUILTIN", "CHAR",  "LGL", # 6-10
    "INT", "REAL", "CPLX", "STR", "DOT", "ANY",   # 13-18
    "VEC", "EXPR", "BCODE", "EXTPTR", "WEAKREF",  # 19-23
    "RAW", "S4", "..."))                          # 24-25, 69

humanize_promise_type_vec = function(type) 
  ifelse(is.na(type), "NA", SEXP_TYPES[[type]])

humanize_promise_type_vec2 = function(type) # TODO: kill
  ifelse(is.na(type), "NA",
    ifelse(type == 0, "NIL",
      ifelse(type == 1, "SYM",
        ifelse(type == 2, "LIST",
          ifelse(type == 3, "CLOS",
            ifelse(type == 4, "ENV",
              ifelse(type == 5, "PROM",
                ifelse(type == 6, "LANG",
                  ifelse(type == 7, "SPECIAL",
                    ifelse(type == 8, "BUILTIN",
                      ifelse(type == 9, "CHAR",
                        ifelse(type == 10, "LGL",
                          ifelse(type == 13, "INT",
                            ifelse(type == 14, "REAL",
                              ifelse(type == 15, "CPLX",
                                ifelse(type == 16, "STR",
                                  ifelse(type == 17, "DOT",
                                    ifelse(type == 18, "ANY",
                                      ifelse(type == 19, "VEC",
                                        ifelse(type == 20, "EXPR",
                                          ifelse(type == 21, "BCODE",
                                            ifelse(type == 22, "EXTPTR",
                                              ifelse(type == 23, "WEAKREF", 
                                                ifelse(type == 24,"RAW", 
                                                  ifelse(type == 25, "S4", 
                                                    ifelse(type == 69, "...", NA))))))))))))))))))))))))))

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
    transform(number=ifelse(is.na(number), 0, number)) %>%
    arrange(type, no.of.forces) %>%
    transform(type=humanize_promise_type_vec(type)) %>%
    mutate(percent_within_type=((number*100/promise_type_count[[type]]))) %>%
    mutate(percent_overall=((number*100/n.promise.forces)))
  
  histogram
}

get_functions_by_type <- function() {
  functions %>% 
    group_by(type) %>% count %>% rename(number=n) %>% 
    transform(type=humanize_function_type(type), percent=100*number/n.functions)
}

get_calls_by_type <- function() {
  left_join(calls, select(functions, function_id, type), by="function_id") %>% 
    group_by(type) %>% count %>% rename(number=n) %>% 
    transform(type=humanize_function_type(type), percent=100*number/n.calls)
}

get_function_compilation_histogram <- function() {
  functions %>% 
    group_by(compiled) %>% count %>% rename(number=n) %>% 
    transform(compiled=as.logical(compiled), percent=100*number/n.functions)
}

get_call_compilation_histogram <- function() {
  left_join(calls, select(functions, function_id, type), by="function_id") %>% 
    group_by(compiled) %>% count %>% rename(number=n) %>% 
    transform(compiled=as.logical(compiled), percent=100*number/n.calls)
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
    transform(type=humanize_function_type(type), compiled=as.logical(compiled)) %>% 
    transform(percent_overall=100*number/n.functions) %>%
    transform(percent_within_type=100*number/functions_by_type_hashmap[[type]])
  
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
    transform(type=humanize_function_type(type)) %>% 
    transform(compiled=ifelse(as.logical(compiled), "compiled", "uncompiled")) %>% 
    transform(percent_overall=100*number/n.calls) %>%
    transform(percent_within_type=100*number/calls_by_type_hashmap[[type]])
  
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
    transform(compiled=ifelse(compiled_runs == 0, 0, as.character(ifelse(runs == compiled_runs, 1, ifelse(compiled_runs == runs - 1, 2, 3))))) %>% 
    group_by(type, compiled) %>% count %>% rename(number=n) %>%
    transform(compiled=ifelse(compiled == 1, "compiled", ifelse(compiled == 2, "after 1st", ifelse(compiled == 0, "uncompiled", "erratic")))) %>%
    transform(type=humanize_function_type(type)) %>%
    transform(percent_overall=100*number/n.functions) %>%
    transform(percent_within_type=100*number/functions_by_type_hashmap[[type]])
    
  if (is.na(specific_type))
    histogram
  else
    histogram %>% select(-type, -percent_overall)
}

get_promise_evaluation_histogram <- function(cutoff=NA) {
  data <- promises %>% rename(promise_id = id) %>% left_join(promise_evaluations, by="promise_id") %>% collect
  unevaluated <- tibble(no.of.evaluations=0, number=(data %>% filter(is.na(event_type)) %>% count %>% data.frame)$n)
  evaluated <- data %>% filter(!is.na(event_type)) %>% group_by(promise_id) %>% count %>% group_by(n) %>% count %>% ungroup %>% rename(no.of.evaluations=n, number=nn)
  
  new.data <- rbind(unevaluated, evaluated) 
  
  if (is.na(cutoff)) {
    new.data %>% mutate(percent=(number*100/n.promises))
  } else {
    above <- new.data %>% filter(no.of.evaluations > cutoff) %>% ungroup %>% collect %>% summarise(no.of.evaluations=Inf, number=sum(number))
    below <- new.data %>% filter(no.of.evaluations <= cutoff)
    rbind(above, below) %>% mutate(percent=(number*100/n.promises))
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
    transform(recursion_rate = classify(percent_repeating)) %>%
    group_by(recursion_rate) %>% 
    summarize(
      number=n(), 
      percent=100*n()/n.functions) %>%
    as.data.frame
  
  data.frame(recursion_rate=classification_table) %>% 
    left_join(histogram, by="recursion_rate") %>% 
    transform(
      number=ifelse(is.na(number), 0, number), 
      percent=ifelse(is.na(percent), 0, percent))
  
}

