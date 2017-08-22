library(dplyr)

packages <- installed.packages()[, "Package"]

#vignettes.in.package <- vignette(package = package)$results[,3]


filter_irrelecant_code <- function(source) {
  content <- source 
  
  index.el <- content %>% grepl("^[ \t]*$", .) # Remove empty lines
  content <- content[!index.el]
  
  index.cm <- content %>% grepl("^#", .) # Remove comments
  content <- content[!index.cm]
  
  content
}

count_source_lines <- function(paths) {
  result <- 0
  for (path in paths)
    result <- result + length(filter_irrelecant_code(path))
}

package_vignette_summary <- function(vignette_summary_data=NA) {
  #tibble(package = packages) %>% rowwise %>% mutate(n.vignettes=length(vignette(package = package)$results[, "Item"]))
  data <- if(suppressWarnings(is.na(vignette_summary_data))) vignette_summary() else vignette_summary_data 
  
  data %>% group_by(package) %>% summarise(n.vignettes=sum(as.integer(!is.na(name))), n.lines=sum(ifelse(is.na(lines), 0, lines)))
}

vignette_summary <- function() {
  result.vignettes <- tibble(index=list(), package=list(), name=list(), source=list(), lines=list())
  first = TRUE
  
  for(package in packages) {
    vignettes <- vignette(package = package)$results
    
    if (length(vignettes) == 0)
      next
    
    vignettes.information <- 
        tibble(index=1:length(vignettes[, "Item"]), package = vignettes[, "Package"], name = vignettes[, "Item"]) %>%
        rowwise %>% mutate(source={vignette.info <- vignette(name, package = package);  file.path(vignette.info$Dir, "doc", vignette.info$R)}) %>%
        mutate(lines = length(filter_irrelecant_code(readLines(source))))
    
    if (first) {
      result.vignettes <- vignettes.information
      first <- FALSE
    } else 
      result.vignettes <- rbind(result.vignettes, vignettes.information)
  }
  
  tibble(package=packages) %>% left_join(result.vignettes, by="package")
}

# library(tibble)
# summary.vignettes <- vignette_summary()
# summary.packages <- package_vignette_summary(summary.vignettes)
# saveRDS(summary.vignettes, "summary_vignettes.rds")
# saveRDS(summary.packages, "summary_vignettes.rds")

# library(tibble)
# summary.vignettes <- readRDS("summary_vignettes.rds"))
# summary.packages <- readRDS("summary_packages.rds"))
# 
