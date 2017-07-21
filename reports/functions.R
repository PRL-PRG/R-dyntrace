library(dplyr)
library(igraph)

if(!exists("path"))
  path <- "/home/kondziu/workspace/R-dyntrace-2/data/rivr.sqlite"

db <- src_sqlite(path)

# tables
promises <- db %>% tbl("promises")
promise_evaluations <- db %>% tbl("promise_evaluations")
promise_associations <- db %>% tbl("promise_associations")
calls <- db %>% tbl("calls") %>% rename(call_id = id)
functions <- db %>% tbl("functions") %>% rename(function_id = id)
arguments <- db %>% tbl("arguments")

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

humanize_promise_type = function(type) 
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

get_lookup_histogram <- function(cutoff=NA) {
  data <- promises %>% rename(promise_id = id) %>% left_join(promise.lookups, by="promise_id") %>% collect
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
  data <- promises %>% rename(promise_id = id) %>% left_join(promise.forces, by="promise_id") %>% collect
  unevaluated <- tibble(no.of.forces=0, number=(data %>% filter(is.na(event_type)) %>% count %>% data.frame)$n)
  evaluated <- data %>% filter(!is.na(event_type)) %>% group_by(promise_id) %>% count %>% group_by(n) %>% count %>% ungroup %>% rename(no.of.forces=n, number=nn) 
  
  new.data <- rbind(unevaluated, evaluated)
  
  if (is.na(cutoff)) {
    new.data
  } else {
    above <- new.data %>% filter(no.of.forces > cutoff) %>% ungroup %>% collect %>% summarise(no.of.forces=Inf, number=sum(number))
    below <- new.data %>% filter(no.of.forces <= cutoff)
    rbind(above, below)
  }
}

get_promise_evaluation_histogram <- function(cutoff=NA) {
  data <- promises %>% rename(promise_id = id) %>% left_join(promise_evaluations, by="promise_id") %>% collect
  unevaluated <- tibble(no.of.evaluations=0, number=(data %>% filter(is.na(event_type)) %>% count %>% data.frame)$n)
  evaluated <- data %>% filter(!is.na(event_type)) %>% group_by(promise_id) %>% count %>% group_by(n) %>% count %>% ungroup %>% rename(no.of.evaluations=n, number=nn)
  
  new.data <- rbind(unevaluated, evaluated)
  
  if (is.na(cutoff)) {
    new.data
  } else {
    above <- new.data %>% filter(no.of.evaluations > cutoff) %>% ungroup %>% collect %>% summarise(no.of.evaluations=Inf, number=sum(number))
    below <- new.data %>% filter(no.of.evaluations <= cutoff)
    rbind(above, below)
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

get_recursiveness_of_calls <- function() {
  parent.calls <- calls %>% mutate(parent_id = call_id, parent_function_id=function_id) %>% select(parent_id, parent_function_id)  
  calls %>% left_join(parent.calls, by="parent_id")
  
}