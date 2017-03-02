f <- function(x) x
g <- function(y) f(y)

test1 <- function() g(2)

ex2 <- function() {
	stop("!")
}

ex1 <- function() {
	ex2()
}

testEx1 <- function() {
	tryCatch(ex1(),
		error = function(c) "error",
		warning = function(c) "warning",
		message = function(c) "message"
	)
}

testEx2 <- function() {
	try(ex1())
}

dddFun <- function(x, ...) {
	dots <- list(...)
	print(dots)
}

testDdd <- function() {
	dddFun(1, 2, 3, 4)
}
