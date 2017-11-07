dyntrace <- function(dyntracer, expr, env = environment()) {
   if(missing(dyntracer)) stop("dyntracer required")
   if(missing(expr)) stop("expression required")
   .Primitive("dyntrace")(dyntracer, expr, env)
}
