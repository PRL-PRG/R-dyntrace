#!/usr/bin/Rscript

compiler::setCompilerOptions(suppressAll = TRUE)

suppressPackageStartupMessages(library("optparse"))
suppressPackageStartupMessages(library("dplyr"))
suppressPackageStartupMessages(library("stringr"))
suppressPackageStartupMessages(library("rmarkdown"))

Sys.setenv(RSTUDIO_PANDOC="/usr/lib/rstudio/bin/pandoc")

PATH_TO_TEMPLATE="/home/kondziu/R-dyntrace/reports/template.Rmd"
PATH_TO_ENGINE="/home/kondziu/R-dyntrace/reports/functions.R"

option_list <- list( 
  make_option(c("-a", "--author"), action="store", type="character", default="",
              help="Report author", metavar="author"),
  make_option(c("--output-format"), action="store", type="character", default="html_document",
              help="Report output format", metavar="format"),
  make_option(c("-d", "--date"), action="store", type="character", default=format(Sys.time(), "%d %B %Y"),
              help="Report date", metavar="date"),
  make_option(c("-c", "--comment"), action="store", type="character", default=NULL,
              help="Additional comment, included in title", metavar="comment"),
  make_option(c("--debug"), action="store_true", default=FALSE,
              help="print debug information [default]", metavar="debug"),
  make_option(c("-e", "--output-extension"), action="store", type="character", default="Rmd",
              help="Report document extension", metavar="extension"),
  make_option(c("-o", "--output-path"), action="store", type="character", default="",
              help="Report document output directory (default is same as --template)", metavar="output_path"),
  make_option(c("--compile"), action="store_true", default=FALSE,
              help="compile Rmd files [default]", metavar="compile"),
  make_option(c("--engine"), action="store", default=PATH_TO_ENGINE,
              help="path to functions.R file [default]", metavar="engine"),
  make_option(c("--template"), action="store", default=PATH_TO_TEMPLATE,
              help="path to an Rmd template file [default]", metavar="template")
)

cfg <- parse_args(OptionParser(option_list=option_list), positional_arguments=TRUE)

print(cfg)

if (cfg$options$`output-path` == "") {
  cfg$options$`output-path` <- dirname(cfg$options$template)
}

make_metadata_line <- function(key, value, comment=NULL, quote=FALSE) 
  paste(key, 
        ": ", 
        if (quote) '"' else "", 
        if(is.null(comment)) value else paste(value, " (", comment, ")", sep=""), 
        if (quote) '"' else "",
        sep="")
make_metadata_segment <- function(...) paste("---", ..., "---\n", sep="\n")

if (cfg$options$debug) 
  print(cfg, stderr())

for (argument in cfg$args){
  package_name <- tools:::file_path_sans_ext(argument) %>% basename
  output_path <- paste(cfg$options$`output-path`, "/", package_name, ".", cfg$options$`output-extension`, sep="")
  
  if (cfg$options$debug)
    print(output_path)
  
  metadata <- make_metadata_segment(make_metadata_line("title", package_name, cfg$options$comment, quote=TRUE),
                                    make_metadata_line("author", cfg$options$author, quote=TRUE),
                                    make_metadata_line("date", cfg$options$date, quote=TRUE),
                                    make_metadata_line("output", cfg$options$`output-format`))
  
  if (cfg$options$debug) 
    print(metadata)
  
  markdown <- 
    readLines(cfg$options$template) %>% paste(collapse="\n") %>% 
    str_replace_all("%%PATH%%", argument) %>% 
    str_replace_all("%%FUNCTIONS%%", cfg$options$engine)
  
  content <- paste(metadata, markdown, sep="\n")
  
  if (cfg$options$debug)
    print(content)
  
  write(content, output_path)
  
  if(cfg$option$compile)
    rmarkdown::render(output_path)
}
