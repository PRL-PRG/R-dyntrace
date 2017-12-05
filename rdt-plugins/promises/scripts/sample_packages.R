#!/usr/bin/Rscript

suppressPackageStartupMessages(library(dplyr))
suppressPackageStartupMessages(library(optparse))

FILTER_ALL = "all"
FILTER_VIGNETTES = "has-vignettes"
FILTER_NONEMPTY_VIGNETTES = "has-non-empty-vignettes"

SAMPLE_JUST_ALPHABETICAL = "alpha"
SAMPLE_RANDOM_ALPHABETICAL = "random-alpha"
SAMPLE_RANDOM = "random"

select_sample <- function (sample_size=NA, criterion=FILTER_ALL, sample_arrangement=SAMPLE_RANDOM) {
  all_packages <- installed.packages()[, "Package"]
  
  filter_irrelecant_code <- function(source) {
    content <- source 
    
    index.el <- content %>% grepl("^[ \t]*$", .) # Remove empty lines
    content <- content[!index.el]
    
    index.cm <- content %>% grepl("^#", .) # Remove comments
    content <- content[!index.cm]
    
    content
  }
  
  result.vignettes <- tibble(index=list(), package=list(), name=list(), source=list(), lines=list())
  first = TRUE
  
  # Get information per vignette
  for(package in all_packages) {
    vignettes <- vignette(package = package)$results
    
    if (length(vignettes) == 0)
      next
    
    vignettes.information <- 
        tibble(index=1:length(vignettes[, "Item"]), 
               package = vignettes[, "Package"], 
               name = vignettes[, "Item"]) %>%
        rowwise %>% 
      mutate(lines={
        vignette.info <- vignette(name, package = package);  
        length(filter_irrelecant_code(readLines(file.path(vignette.info$Dir, "doc", vignette.info$R))))})
    
    if (first) {
      result.vignettes <- vignettes.information
      first <- FALSE
    } else {
      result.vignettes <- rbind(result.vignettes, vignettes.information)
    }
  }
  
  # Aggregate vignettes to get package information
  package_information <- 
    tibble(package=all_packages) %>% left_join(result.vignettes, by="package") %>% 
    group_by(package) %>% summarise(
      n.vignettes=sum(as.integer(!is.na(name))), 
      n.lines=sum(ifelse(is.na(lines), 0, lines)))
  
  # Filter by type
  filtered_package_information <- if (criterion==FILTER_ALL) {
    package_information
  } else if (criterion == FILTER_VIGNETTES) {
    package_information %>% filter(n.vignettes > 0)
  } else if (criterion == FILTER_NONEMPTY_VIGNETTES) {
    package_information %>% filter(n.lines > 0)
  } else {
    stop(paste("Unknown sample pre-selection criterion:", criterion))
  } %>% as.data.frame
  
  # Figure out the dataset size vis-avis the sample size and adjust
  dataset_size <- nrow(filtered_package_information)
  
  if (is.na(sample_size)) {
    sample_size <- dataset_size
  } else if (sample_size > dataset_size) {
    sample_size <- dataset_size
    warning("Sample size greater than dataset size, cutting it down to dataset size.")
  }
  
  # Sampling
  a_sample <- if(sample_arrangement == SAMPLE_RANDOM) {
    sample_vector <- sample(x = dataset_size, size = sample_size)
    filtered_package_information[sample_vector,]
  } else if (sample_arrangement == SAMPLE_RANDOM_ALPHABETICAL) {
    sample_vector <- sample(x = dataset_size, size = sample_size)
    filtered_package_information[sample_vector,] %>% arrange(package)
  } else if (sample_arrangement == SAMPLE_JUST_ALPHABETICAL) {
    filtered_package_information %>% head(sample_size) %>% arrange(package)
  } else {
    stop(paste("Unknown sample arrangement:", sample_arrangement))
  }
  
  return(a_sample)
}

# Main starts here

suppressWarnings(
  option_list <- list(
    make_option(c("-n", "--sample-size"),
                action="store",
                type="numeric",
                default="NA",
                help="Sample size or nothing for all",
                metavar="sample"),
    make_option(c("-a", "--sample-arrangement"),
                action="store",
                type="character",
                default="random",
                help="Selection and arranegment of packages in sample: random, alpha, random-alpha",
                metavar="arrangement"),
    make_option(c("-f", "--pre-sample-filter"),
                action="store",
                type="character",
                default="all",
                help="Pre-selection criterion: all, has-vignettes, has-non-empty-vignettes",
                metavar="criterion")
  )
)

# Process commandline options
cfg <- parse_args(OptionParser(option_list=option_list), positional_arguments=TRUE)

# Select the sample
selected_sample <- select_sample(
  sample_size = cfg$options$`sample-size`,
  sample_arrangement = cfg$options$`sample-arrangement`,
  criterion = cfg$options$`pre-sample-filter`
)

# Prepare printable version:
printable_sample <- 
  selected_sample %>% 
  mutate(summary=paste(package, n.vignettes, n.lines, sep=";")) %>% 
  pull(summary)
write(printable_sample, stdout())
