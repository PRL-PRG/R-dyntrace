#ifndef __RECORDER_HPP__
#define __RECORDER_HPP__

#include <chrono>
#include <ctime>
#include <inspect.h>
#include <tuple>

//#include <Defn.h> // We need this for R_Funtab
#include "tracer_sexpinfo.h"
#include "tracer_state.h"
#include <rdt.h>
#include <set>

void get_environment_metadata(metadata_t &metadata);
void get_current_time_metadata(metadata_t &metadata, string prefix);
closure_info_t function_entry_get_info(const SEXP call,
                                       const SEXP op,
                                       const SEXP rho);
closure_info_t function_exit_get_info(const SEXP call,
                                      const SEXP op,
                                      const SEXP rho);
builtin_info_t builtin_entry_get_info(const SEXP call,
                                      const SEXP op,
                                      const SEXP rho,
                                      function_type fn_type);
builtin_info_t builtin_exit_get_info(const SEXP call,
                                     const SEXP op,
                                     const SEXP rho,
                                     function_type fn_type);
prom_basic_info_t create_promise_get_info(const SEXP promise, const SEXP rho);
prom_info_t force_promise_entry_get_info(const SEXP symbol, const SEXP rho);
prom_info_t force_promise_exit_get_info(const SEXP symbol,
                                        const SEXP rho,
                                        const SEXP val);
prom_info_t promise_lookup_get_info(const SEXP symbol,
                                    const SEXP rho,
                                    const SEXP val);
prom_info_t promise_expression_lookup_get_info(const SEXP prom,
                                               const SEXP rho);
gc_info_t gc_exit_get_info(int gc_count, double vcells, double ncells);
#endif /* __RECORDER_HPP__ */
