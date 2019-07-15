---
layout: default
---

**R-dyntrace** is a dynamic tracing framework for R. It is a modified GNU R Virtual Machine that exposes the interpreter internals through probes. 
Callbacks attached to these probes are called with the relevant interpreter state when the corresponding probes get triggered.


## Getting Started

It is recommended to build this project locally and not install it system-wide.
To download and compile version _3.5.0_:

```
git clone --branch r-3.5.0 https://github.com/PRL-PRG/R-dyntrace.git
cd R-dyntrace
./configure --enable-R-shlib --with-blas --with-lapack --with-included-gettext --disable-byte-compiled-packages
make -j
```

## Supported Versions

**Version** | **Source** | **Status**
------------|------------|-----------
[R-3.5.0](https://github.com/PRL-PRG/R-dyntrace/tree/r-3.5.0 "R-3.5.0 Branch")| [R-3.5.0](https://github.com/PRL-PRG/R-dyntrace/archive/r-3.5.0.zip "R-3.5.0 ZIP")  | [![Build Status](https://travis-ci.org/PRL-PRG/R-dyntrace.svg?branch=r-3.5.0)](https://travis-ci.org/PRL-PRG/R-dyntrace) 
[R-3.5.1](https://github.com/PRL-PRG/R-dyntrace/tree/r-3.5.1 "R-3.5.1 Branch")| [R-3.5.1](https://github.com/PRL-PRG/R-dyntrace/archive/r-3.5.1.zip "R-3.5.1 ZIP")  | [![Build Status](https://travis-ci.org/PRL-PRG/R-dyntrace.svg?branch=r-3.5.1)](https://travis-ci.org/PRL-PRG/R-dyntrace) 
[R-3.5.2](https://github.com/PRL-PRG/R-dyntrace/tree/r-3.5.2 "R-3.5.2 Branch")| [R-3.5.2](https://github.com/PRL-PRG/R-dyntrace/archive/r-3.5.2.zip "R-3.5.2 ZIP")   | [![Build Status](https://travis-ci.org/PRL-PRG/R-dyntrace.svg?branch=r-3.5.2)](https://travis-ci.org/PRL-PRG/R-dyntrace) 
[R-3.5.3](https://github.com/PRL-PRG/R-dyntrace/tree/r-3.5.3 "R-3.5.3 Branch")| [R-3.5.3](https://github.com/PRL-PRG/R-dyntrace/archive/r-3.5.3.zip "R-3.5.3 ZIP")   | [![Build Status](https://travis-ci.org/PRL-PRG/R-dyntrace.svg?branch=r-3.5.3)](https://travis-ci.org/PRL-PRG/R-dyntrace) 
[R-3.6.0](https://github.com/PRL-PRG/R-dyntrace/tree/r-3.6.0 "R-3.6.0 Branch")| [R-3.6.0](https://github.com/PRL-PRG/R-dyntrace/archive/r-3.6.0.zip "R-3.6.0 ZIP")   | [![Build Status](https://travis-ci.org/PRL-PRG/R-dyntrace.svg?branch=r-3.6.0)](https://travis-ci.org/PRL-PRG/R-dyntrace) 
[R-3.6.1](https://github.com/PRL-PRG/R-dyntrace/tree/r-3.6.1 "R-3.6.1 Branch")| [R-3.6.1](https://github.com/PRL-PRG/R-dyntrace/archive/r-3.6.1.zip "R-3.6.1 ZIP")   | [![Build Status](https://travis-ci.org/PRL-PRG/R-dyntrace.svg?branch=r-3.6.1)](https://travis-ci.org/PRL-PRG/R-dyntrace)


## Details

The framework supports the following probes.


### 1. Setup/Cleanup Probes

| Event         | Probe                                                 |
|---------------|-------------------------------------------------------|
| Tracing entry | dyntrace_entry(expression, environment)               |
| Tracing exit  | dyntrace_exit(expression, environment, result, error) |


### 2. Function Probes

| Event         | Probe                               |
|---------------|-------------------------------------|
| Closure entry | closure_entry(call, op, rho)        |
| Closure exit  | closure_exit(call, op, rho, retval) |
| Builtin entry | builtin_entry(call, op, rho)        |
| Builtin exit  | builtin_exit(call, op, rho, retval) |
| Special entry | special_entry(call, op, rho)        |
| Special exit  | special_exit(call, op, rho, retval) |


### 3. Promise Probes

| Event                      | Probe                                            |
|----------------------------|--------------------------------------------------|
| Promise force entry        | promise_force_entry(promise)                     |
| Promise force exit         | promise_force_exit(promise)                      |
| Promise value lookup       | promise_value_lookup(promise)                    |
| Promise value assign       | promise_value_assign(promise, value)             |
| Promise expression lookup  | promise_expression_lookup(promise)               |
| Promise expression assign  | promise_expression_assign(promise, expression)   |
| Promise environment lookup | promise_environment_lookup(promise)              |
| Promise environment assign | promise_environment_assign(promise, environment) |


### 4. Eval Probes

| Event      | Probe                                    |
|------------|------------------------------------------|
| Eval entry | eval_entry(expression, rho)              |
| Eval exit  | eval_exit(expression, rho, return_value) |


### 5. GC Probes

| Event       | Probe                 |
|-------------|-----------------------|
| GC entry    | gc_entry(size_needed) |
| GC exit     | gc_exit(gc_count)     |
| GC unmark   | gc_unmark(object)     |
| GC allocate | gc_allocate(object)   |


### 6. Context Probes

| Event         | Probe                                        |
|---------------|----------------------------------------------|
| Context entry | context_entry(context)                       |
| Context exit  | context_exit(context)                        |
| Context jump  | context_jump(context, return_value, restart) |


### 7. S3 Probes

| Event             | Probe                                                  |
|-------------------|--------------------------------------------------------|
| S3 entry          | S3_generic_entry(generic, object)                      |
| S3 exit           | S3_generic_exit(generic, object, retval)               |
| S3 dispatch entry | S3_dispatch_entry(generic, cls, method, object)        |
| S3 dispatch exit  | S3_dispatch_exit(generic, cls, method, object, retval) |


### 8. Environment Probes

| Event               | Probe                                           |
|---------------------|-------------------------------------------------|
| Variable definition | environment_variable_define(symbol, value, rho) |
| Variable assignment | environment_variable_assign(symbol, value, rho) |
| Variable removal    | environment_variable_remove(symbol, rho)        |
| Variable lookup     | environment_variable_lookup(symbol, value, rho) |
| Variable existence  | environment_variable_exists(symbol, rho)        |


## Talks

1. **[RDT: A Dynamic Tracing Framework for R](https://docs.google.com/presentation/d/1UTMvBIv1Y1ZFG-jXbD28G-Z8oUSB3vgPoSjRAIniurw/edit?usp=sharing "RDT: A Dynamic Tracing Framework for R")** <br/>
[Aviral Goel](http://aviral.io/ "Aviral Goel") • [Filip Krikava](http://fikovnik.net/ "Filip Krikava") • [Jan Vitek](http://janvitek.org/ "Jan Vitek")<br/>
@ [RIOT 2019](https://riotworkshop.github.io/ "RIOT 2019") @ [useR! 2019](http://www.user2019.fr/ "useR! 2019")

## Writeups

1. **[RDT: A Dynamic Tracing Framework for R](https://riotworkshop.github.io/abstracts/riot-2019-dynamic-tracing.pdf "RDT: A Dynamic Tracing Framework for R")** <br/>
[Aviral Goel](http://aviral.io/ "Aviral Goel") • [Filip Krikava](http://fikovnik.net/ "Filip Krikava") • [Jan Vitek](http://janvitek.org/ "Jan Vitek")<br/>
@ [RIOT 2019](https://riotworkshop.github.io/ "RIOT 2019") @ [useR! 2019](http://www.user2019.fr/ "useR! 2019")

## Publications

1. **On the Design, Implementation and Use of Laziness in R**<br/>
[Aviral Goel](http://aviral.io/ "Aviral Goel") • [Jan Vitek](http://janvitek.org/ "Jan Vitek")<br/>
@ [OOPSLA 2019](https://2019.splashcon.org/track/splash-2019-oopsla "Object-Oriented Programming, Systems, Languages & Applications 2019") (conditionally accepted)

