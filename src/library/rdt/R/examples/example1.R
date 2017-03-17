f <- function(x) x
g <- function(x,y,z) {x+y+z}
h <- function(v,w) {v+1+w}
i <- function(x,y,...) {h(v=x,w=y) + h(...)}
j <- function(x,y,z) x + h(v=z,w=z)
k <- function(...) { l<-list(...); f(l) }
l <- function(...) { l<-list(...); print(l) }
trace.promises.sql({f(1); f(2); f(3); f(4); g(1+1,2+2,3+3); h(1,1); j(1,1,1); j(1,1,1); i(1,1,1,1); l(1,2,3); k(1,2,3);})