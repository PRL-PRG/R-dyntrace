f <- function(x) x
g <- function(y) f(y)

test1 <- function() g(2)

ex2 <- function() {
	stop("!")
}

ex1 <- function() {
	ex2()
}

testEx <- function() {
	tryCatch(ex1(),
		error = function(c) "error",
		warning = function(c) "warning",
		message = function(c) "message"
	)
}
