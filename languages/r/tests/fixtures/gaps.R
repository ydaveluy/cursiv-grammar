# Exercises constructs the upstream corpus (example/) does not: exponent,
# component/namespace access, both spellings of and/or, model formulae,
# every assignment arrow, hex/float/complex literals, the constant keywords,
# and the `...`/`.` placeholders in formal and actual argument lists.
x <- 2^10
p <- TRUE && FALSE
q <- TRUE & FALSE
r <- TRUE || FALSE
obj <- list(a = 1)
slot1 <- obj$a
slot2 <- obj@a
alias <- base:::sum
formula1 <- ~x + y
formula2 <- y ~ x
n1 <- NULL
n2 <- NA
n3 <- inf
n4 <- NaN
hexv <- 0x1F
floatv <- 3.14e2
cplxv <- 2i
10 -> a
1 ->> b
a <<- 2
c := 3

f <- function(...) sum(...)
g <- function(x = 1, ...) x
h <- function(.) 1

m <- matrix()
elided <- m[, 1]
namedEmpty <- f(x =, y = 2)
