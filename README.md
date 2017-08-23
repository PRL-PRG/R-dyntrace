# R-dyntrace
Dynamic tracing support for R

# Build status
[![Build Status](https://travis-ci.org/PRL-PRG/R-dyntrace.svg?branch=master)](https://travis-ci.org/PRL-PRG/R-dyntrace)

# Commands
```
Rdt(tracer="promises", block={1+1}, format="sql")
trace.promises.r({1+1})
trace.promises.sql({1+1}) # save to file = trace.sql
trace.promises.sql({1+1}, output=CONSOLE) #other outputs are FILE, DB
trace.promises.db({1+1}) # tracer.sqlite uses PSQL
trace.promises.uncompiled.db({1+1}) # tracer.sqlite uses SQL
run_vignettes.sh
run_vignettes.R
compose_testable_vignettes.R
data/doc/
./run_vignettes.sh rivr grid dplyr (edited)
./dbg_vignettes.sh rivr grid dplyr
r-package-anthology.csv  r-package-anthology.r
```
