# R-dyntrace

[![Travis build status](https://api.travis-ci.org/PRL-PRG/R-dyntrace.svg?branch=r-3.5.0)](https://travis-ci.org/PRL-PRG/R-dyntrace)

R-dyntrace is a modified version of R interpreter that contains probes embedded inside it. These probes get called at key events inside the interpreter with relevant interpreter state information. Currently, R-3.5.0 is supported.


## Probes

Currently supported probes and the corresponding events are tabulated below.


### Setup/Cleanup Probes

| Event         | Probe                                                 |
|---------------|-------------------------------------------------------|
| Tracing entry | dyntrace_entry(expression, environment)               |
| Tracing exit  | dyntrace_exit(expression, environment, result, error) |


### Function Probes

| Event         | Probe                               |
|---------------|-------------------------------------|
| Closure entry | closure_entry(call, op, rho)        |
| Closure exit  | closure_exit(call, op, rho, retval) |
| Builtin entry | builtin_entry(call, op, rho)        |
| Builtin exit  | builtin_exit(call, op, rho, retval) |
| Special entry | special_entry(call, op, rho)        |
| Special exit  | special_exit(call, op, rho, retval) |


### Promise Probes

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


### Eval Probes

| Event      | Probe                                    |
|------------|------------------------------------------|
| Eval entry | eval_entry(expression, rho)              |
| Eval exit  | eval_exit(expression, rho, return_value) |


### GC Probes

| Event       | Probe                 |
|-------------|-----------------------|
| GC entry    | gc_entry(size_needed) |
| GC exit     | gc_exit(gc_count)     |
| GC unmark   | gc_unmark(object)     |
| GC allocate | gc_allocate(object)   |


### Context Probes

| Event         | Probe                                        |
|---------------|----------------------------------------------|
| Context entry | context_entry(context)                       |
| Context exit  | context_exit(context)                        |
| Context jump  | context_jump(context, return_value, restart) |


### S3 Probes

| Event             | Probe                                                  |
|-------------------|--------------------------------------------------------|
| S3 entry          | S3_generic_entry(generic, object)                      |
| S3 exit           | S3_generic_exit(generic, object, retval)               |
| S3 dispatch entry | S3_dispatch_entry(generic, cls, method, object)        |
| S3 dispatch exit  | S3_dispatch_exit(generic, cls, method, object, retval) |


### Environment Probes

| Event               | Probe                                           |
|---------------------|-------------------------------------------------|
| Variable definition | environment_variable_define(symbol, value, rho) |
| Variable assignment | environment_variable_assign(symbol, value, rho) |
| Variable removal    | environment_variable_remove(symbol, rho)        |
| Variable lookup     | environment_variable_lookup(symbol, value, rho) |
| Variable existence  | environment_variable_exists(symbol, rho)        |


## Building

It is recommended to build this project locally and not install it system-wide.
Doing otherwise will have unknown consequences.

To download and build this locally, run the following commands -

```
$ git clone git@github.com:PRL-PRG/R-dyntrace.git
$ cd R-dyntrace
$ ./build
```
