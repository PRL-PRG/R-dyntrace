R-dyntrace
=============

[![Travis build status](https://travis-ci.org/PRL-PRG/R-dyntrace.svg?branch=master)](https://travis-ci.org/PRL-PRG/R-dyntrace)

Overview
----------
Dynamic tracing for R.

Design Considerations
--------------------------

1. Nested hook execution is disallowed by the framework. Hooks intercept
   execution of R interpreter code. If one hook triggers another, then the
   triggered hook is intercepting code that is executed by the hook and not by
   the program being traced. This is likely not what you intended to do. When
   such a situation arises, the framework prints the name of the currently
   active hook and of the hook being triggered by it; and exits.

2. Garbage collection is turned off by the framework before entry into the hook
   and reinstated upon exit. This reduces the probability of the hook code
   triggering garbage collection. The hook code can still rely on the
   interpreter's C code which can turn on garbage collection. Hence, the
   mechanism is not foolproof and care has to taken when calling the interpreter
   code. Even if such a situation happens, the framework will detect it and stop
   the execution of the program. This is because when the garbage collector is
   triggered, a hook inside the garbage collector gets triggered. This nested
   hook execution is detected by the framework.

Build
------

```bash
# to build the R interpreter and the tracer
./build.sh

# to build the tracer
./build-tracer.sh
```

Usage
------

```r
Rdt(tracer="promises", block={1+1}, format="sql")

trace.promises.r({1+1})

trace.promises.sql({1+1}) # save to file = trace.sql

trace.promises.sql({1+1}, output=CONSOLE) #other outputs are FILE, DB

trace.promises.db({1+1}) # tracer.sqlite uses PSQL

trace.promises.uncompiled.db({1+1}) # tracer.sqlite uses SQL
```
