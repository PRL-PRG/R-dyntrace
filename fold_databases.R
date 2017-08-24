library(hashmap)
library(dplyr)

## TODO make indexes?
fold_databases <- function(result_path, ...) {
  paths = c(...)
  
  if (length(paths) == 0) {
    warning("Nothing to do.")
    return
  }
  
  # Helper functions
  promise_id_mutator <- function(x)
    mutate(x, promise_id = ifelse(promise_id >= 0, promises_id_positive_offset, promises_id_negative_offset) + promise_id)
  
  in_prom_id_mutator <- function(x)
    mutate(x, in_prom_id = ifelse(in_prom_id >= 0, promises_id_positive_offset, promises_id_negative_offset) + in_prom_id)
  
  get_max_id <- function(x) {
    value <- (select(x, id) %>% filter(id >= 0) %>% summarise(max=max(id)) %>% as.data.frame)$max
    if (is.na(value)) 0L else value
  }
  
  get_min_id <- function(x) {
    value <- (select(x, id) %>% filter(id < 0) %>% summarise(min=min(id)) %>% as.data.frame)$min
    if (is.na(value)) 0L else value
  }
  
  write(paste("Concatenating", paths[1], "(copy outright)"), stderr())
  
  # Copy first one outright, use it as Zero.
  file.copy(paths[1], result_path, overwrite=TRUE)
  result <- src_sqlite(result_path)
  zero <- src_sqlite(result_path, create=FALSE)
  
  # Tables in Zero:
  zero.functions              <- zero %>% tbl("functions")            
  zero.calls                  <- zero %>% tbl("calls")                
  zero.arguments              <- zero %>% tbl("arguments")
  zero.promises               <- zero %>% tbl("promises")             
  zero.promise_evaluations    <- zero %>% tbl("promise_evaluations")  
  zero.promise_associations   <- zero %>% tbl("promise_associations") 
  zero.promise_returns        <- zero %>% tbl("promise_returns")
  zero.gc_triggers            <- zero %>% tbl("gc_trigger")
  zero.promise_lifecycles     <- zero %>% tbl("promise_lifecycle")
  zero.type_distributions     <- zero %>% tbl("type_distribution")
  zero.metadata               <- zero %>% tbl("metadata")
  
  # Sizes:
  write("    * sizes: ", stderr())
  
  n.zero.functions            <- zero.functions %>% count %>% pull(n)
  write(paste("        - functions:           ", pp(n.zero.functions)), stderr())
  
  n.zero.calls                <- zero.calls %>% count %>% pull(n)
  write(paste("        - calls:               ", pp(n.zero.calls)), stderr())
  
  n.zero.arguments            <- zero.arguments %>% count %>% pull(n)
  write(paste("        - arguments:           ", pp(n.zero.arguments)), stderr())
  
  n.zero.promises             <- zero.promises %>% count %>% pull(n)
  write(paste("        - promises:            ", pp(n.zero.promises)), stderr())
  
  n.zero.promise_evaluations  <- zero.promise_evaluations %>% count %>% pull(n)
  write(paste("        - promise_evaluations: ", pp(n.zero.promise_evaluations)), stderr())
  
  n.zero.promise_associations <- zero.promise_associations %>% count %>% pull(n)
  write(paste("        - promise_associations:", pp(n.zero.promise_associations)), stderr())
  
  n.zero.promise_returns      <- zero.promise_returns %>% count %>% pull(n)
  write(paste("        - promise_returns:     ", pp(n.zero.promise_returns)), stderr())
  
  n.zero.gc_triggers          <- zero.gc_triggers %>% count %>% pull(n)
  write(paste("        - gc_triggers:         ", pp(n.zero.gc_triggers)), stderr())
  
  n.zero.promise_lifecycles   <- zero.promise_lifecycles %>% count %>% pull(n)
  write(paste("        - promise_lifecycles:  ", pp(n.zero.promise_lifecycles)), stderr())
  
  n.zero.type_distributions   <- zero.type_distributions %>% count %>% pull(n)
  write(paste("        - type_distributions:  ", pp(n.zero.type_distributions)), stderr())
  
  n.zero.metadata             <- zero.metadata %>% count %>% pull(n)
  write(paste("        - type_metadata:       ", pp(n.zero.metadata)), stderr())
  
  # Start the function id dictionary - for Zero it's an identity function.
  write("    * calculating offsets", stderr())
  all.functions <- zero.functions %>% select(location, definition, id) %>% collect
  
  # ID offsets for all other tables:
  call_id_offset <- (zero.calls %>% get_max_id)
  promises_id_positive_offset <- (zero.promises %>% get_max_id)
  promises_id_negative_offset <- (zero.promises %>% get_min_id)
  clock_offset <- (zero.promise_evaluations %>% summarise(max=max(clock)) %>% as.data.frame)$max + 1
  counter_offset <- (zero.gc_triggers %>% summarise(max=max(counter)) %>% as.data.frame)$max
  argument_id_offset <- (zero.arguments %>% get_max_id)
  
  # Fold all subsequent dbs into Zero.
  paths <- paths[2:length(paths)]
  for (path in paths) {
    write(paste("Concatenating", path), stderr())
    
    if (!file.exists(path)) {
      write("    * does not exist, skipping", stderr())
      next
    }
    
    db <- src_sqlite(path, create=FALSE)
    
    # Tables in concatenated DB
    db.functions              <- db %>% tbl("functions")
    db.calls                  <- db %>% tbl("calls")
    db.arguments              <- db %>% tbl("arguments")
    db.promises               <- db %>% tbl("promises")
    db.promise_evaluations    <- db %>% tbl("promise_evaluations")
    db.promise_associations   <- db %>% tbl("promise_associations")
    db.promise_returns        <- db %>% tbl("promise_returns")
    db.gc_triggers            <- db %>% tbl("gc_trigger")
    db.promise_lifecycles     <- db %>% tbl("promise_lifecycle")
    db.type_distributions     <- db %>% tbl("type_distribution")
    db.metadata               <- db %>% tbl("metadata")
    
    # Sizes:
    write("    * sizes: ", stderr())
    
    n.db.functions            <- db.functions %>% count %>% pull(n)
    write(paste("        - functions:           ", pp(n.db.functions)),  stderr())
    
    n.db.calls                <- db.calls %>% count %>% pull(n)
    write(paste("        - calls:               ", pp(n.db.calls)), stderr())
    
    n.db.arguments            <- db.arguments %>% count %>% pull(n)
    write(paste("        - arguments:           ", pp(n.db.arguments)), stderr())
    
    n.db.promises             <- db.promises %>% count %>% pull(n)
    write(paste("        - promises:            ", pp(n.db.promises)), stderr())
    
    n.db.promise_evaluations  <- db.promise_evaluations %>% count %>% pull(n)
    write(paste("        - promise_evaluations: ", pp(n.db.promise_evaluations)),  stderr())
    
    n.db.promise_associations <- db.promise_associations %>% count %>% pull(n)
    write(paste("        - promise_associations:", pp(n.db.promise_associations)), stderr())
    
    n.db.promise_returns      <- db.promise_returns %>% count %>% pull(n)
    write(paste("        - promise_returns:     ", pp(n.db.promise_returns)), stderr())
    
    n.db.gc_triggers          <- db.gc_triggers %>% count %>% pull(n)
    write(paste("        - gc_triggers:         ", pp(n.db.gc_triggers)), stderr())
    
    n.db.promise_lifecycles   <- db.promise_lifecycles %>% count %>% pull(n)
    write(paste("        - promise_lifecycles:  ", pp(n.db.promise_lifecycles)), stderr())
    
    n.db.type_distributions   <- db.type_distributions %>% count %>% pull(n)
    write(paste("        - type_distributions:  ", pp(n.db.type_distributions)), stderr())
    
    n.db.metadata             <- db.metadata %>% count %>% pull(n)
    write(paste("        - type_metadata:       ", pp(n.db.metadata)), stderr())
      
    # Rule of thumb: no functions, nothing interesting inside, so skipping
    if (n.db.functions == 0) {
      write("    * no functions, skipping database", stderr())  
      next
    }
    
    #browse()
    
    # Functions
    write("    * merging functions", stderr())
    functions.dict.all <- 
      all.functions %>% 
      rename(id.zero=id) %>% 
      full_join( #################################### bug here: results in one function being inserted multiple times. too tired to figure out how to fix...
        db.functions %>% 
          select(location, definition, id) %>% 
          rename(id.db=id), 
        by=c("definition", "location"), 
        copy=TRUE) %>% 
      select(id.db, id.zero) %>% 
      collect
    function_id_offset <- (all.functions %>% get_max_id) + 1
    function.exists.in.both.length <-
      functions.dict.all %>%
      filter(!is.na(id.zero)) %>% 
      filter(!is.na(id.db)) %>%
      count %>% rename(number=n) %>% 
      pull(number)
    if (function.exists.in.both.length > 0)
      function.exists.in.both <- 
        functions.dict.all %>% 
        rename(new.id=id.zero, id=id.db) %>%
        filter(!is.na(new.id)) %>% 
        filter(!is.na(id))
    
    function.exists.in.new.length <- 
      (functions.dict.all %>% filter(is.na(id.zero)) %>% count)$n
    if (function.exists.in.new.length > 0)
      function.exists.in.new <- 
        functions.dict.all %>% 
        rename(new.id=id.zero, id=id.db) %>% 
        filter(is.na(new.id)) %>% 
        mutate(new.id=1:function.exists.in.new.length + function_id_offset) # this produces huge holes in the id sequence
    
    
    
    function_id_translation_tbl <- 
      if (function.exists.in.both.length > 0 && function.exists.in.new.length > 0)
        union_all(function.exists.in.both, function.exists.in.new)
      else if (function.exists.in.both.length > 0) 
        function.exists.in.both
      else if (function.exists.in.new.length > 0)
        function.exists.in.new
      else
        function.exists.in.new # EXCEPTION?
    
    if (function.exists.in.new.length > 0) {
      new.functions <- 
        function.exists.in.new %>% 
        left_join(db.functions, by="id", copy=TRUE) %>% 
        select(-id) %>% 
        rename(id=new.id) %>% 
        select(id, location, definition, type, compiled)
      db_insert_into(result$con, "functions", new.functions %>% collect)
    }
    
    if (path == "~/workspace/R-dyntrace/data/ELE-2/stringr.sqlite") browser()
    # paths <- paste("~/workspace/R-dyntrace/data/ELE-2/", c("R6", "Rcpp", "jsonlite", "curl", "tibble", "ggplot2", "dplyr", "rlang", "stringr"), ".sqlite", sep="")
    # fold_databases("/tmp/1.sqlite", paths)
    
    # Calls
    write("    * merging calls", stderr())
    new.calls <- 
      db.calls %>% 
      mutate(
        id = as.integer(ifelse(id == 0, 0, id + call_id_offset)), 
        parent_id = as.integer(ifelse(parent_id == 0, 0, parent_id + call_id_offset))) %>% 
      in_prom_id_mutator %>%
      left_join(function_id_translation_tbl %>% rename(function_id=id), by="function_id", copy=TRUE) %>% 
      select(-function_id) %>% 
      rename(function_id=new.id) %>%
      select(id, function_name, callsite, compiled, function_id, parent_id, in_prom_id, parent_on_stack_type, parent_on_stack_id) # must order the columns to reflect their order in the DB
    db_insert_into(result$con, "calls", new.calls %>% collect)
    
    # Arguments
    write("    * merging arguments", stderr())
    new.arguments <- 
      db.arguments %>%
      mutate(
        id = id + argument_id_offset, 
        call_id = ifelse(call_id == 0, 0, call_id + call_id_offset)) %>%
      select(id, name, position, call_id)
    # todo: push new.arguments to end of zero.arguments in db
    db_insert_into(result$con, "arguments", new.arguments %>% collect)
    
    # Promises
    write("    * merging promises", stderr())
    new.promises <- 
      db.promises %>% 
      rename(promise_id = id) %>% 
      promise_id_mutator %>% 
      in_prom_id_mutator %>%
      rename(id = promise_id) %>%
      select(id,  type, full_type, in_prom_id, parent_on_stack_type, parent_on_stack_id, promise_stack_depth)
    # todo: push new.promises to end of zero.promises in db
    db_insert_into(result$con, "promises", new.promises %>% collect)
    
    # Promise associations
    write("    * merging promise associations", stderr())
    new.promise_associations <- 
      db.promise_associations %>% 
      mutate(
        call_id = ifelse(call_id == 0, 0, call_id_offset + call_id), 
        argument_id = argument_id_offset + argument_id) %>%
      promise_id_mutator %>% 
      select(promise_id, call_id, argument_id)
    # todo: push new.promise_assoc to end of zero.promise_assoc in db
    db_insert_into(result$con, "promise_associations", new.promise_associations %>% collect)
    
    # Promise evaluations
    write("    * merging promise evaluations", stderr())
    new.promise_evaluations <-
      db.promise_evaluations %>% 
      mutate(
        clock = clock_offset + clock, 
        in_call_id = ifelse(in_call_id == 0, 0, call_id_offset + in_call_id), 
        from_call_id = ifelse(from_call_id == 0, 0, call_id_offset + from_call_id)) %>% 
      promise_id_mutator %>% 
      in_prom_id_mutator %>%
      select(clock, event_type, promise_id, from_call_id, in_call_id, in_prom_id, lifestyle, effective_distance_from_origin, actual_distance_from_origin, parent_on_stack_type, parent_on_stack_id, promise_stack_depth)
    # todo: push to db
    db_insert_into(result$con, "promise_evaluations", new.promise_evaluations %>% collect)
    
    # Promise returns
    write("    * merging promise returns", stderr())
    new.promise_returns <- 
      db.promise_returns %>% 
      mutate(clock = clock_offset + clock) %>% 
      promise_id_mutator %>%
      select(type, promise_id, clock)
    # todo: push to db
    db_insert_into(result$con, "promise_returns", new.promise_returns %>% collect)
    
    # GC triggers
    write("    * merging gc triggers", stderr())
    new.gc_triggers <- 
      db.gc_triggers %>%
      mutate(counter = counter_offset + counter) %>%
      select(counter, ncells, vcells)
    # todo: push to db
    db_insert_into(result$con, "gc_trigger", new.gc_triggers %>% collect)
    
    # Promise lifecycles
    write("    * merging promise lifecycles", stderr())
    new.promise_lifecycles <-
      db.promise_lifecycles %>% 
      mutate(gc_trigger_counter = counter_offset + gc_trigger_counter) %>%
      promise_id_mutator %>%
      select(promise_id, event_type, gc_trigger_counter)
    # todo: push to db
    db_insert_into(result$con, "promise_lifecycle", new.promise_lifecycles %>% collect)
    
    # Type distributions
    write("    * merging type distributions", stderr())
    new.type_distributions <-
      db.type_distributions %>% 
      mutate(gc_trigger_counter = counter_offset + gc_trigger_counter) %>%
      select(gc_trigger_counter,  type, length, bytes)
    # todo: push to db
    db_insert_into(result$con, "type_distribution", new.type_distributions %>% collect)
    
    # Metadata
    write("    * merging metadata", stderr())
    new.metadata <- db.metadata
    db_insert_into(result$con, "metadata", new.metadata %>% collect)
    # todo: push to db
    
    # Update all functions id collection
    write("    * calculating offsets", stderr())
    all.functions <- union_all(all.functions, new.functions %>% select(location, definition, id)) 
    
    # Update offsets
    promises_id_positive_offset <- max((new.promises %>% get_max_id), promises_id_positive_offset)
    promises_id_negative_offset <- min((new.promises %>% get_min_id), promises_id_negative_offset)
    call_id_offset <- max((new.calls %>% get_max_id), call_id_offset)
    clock_offset <- max((new.promise_evaluations %>% summarise(max=max(clock)) %>% as.data.frame)$max + 1, clock_offset)
    counter_offset <- max((new.gc_triggers %>% summarise(max=max(counter)) %>% as.data.frame)$max, counter_offset)
    argument_id_offset <- max((new.arguments %>% get_max_id), argument_id_offset)
  }
}