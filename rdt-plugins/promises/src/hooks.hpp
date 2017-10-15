#ifndef __HOOKS_HPP__
#define __HOOKS_HPP__

#include <rdt.h>
#include "globals.hpp"
#include "utilities.hpp"
#include "recorder.hpp"

void begin(const SEXP prom);
void end();
void function_entry(const SEXP call, const SEXP op, const SEXP rho);
void function_exit(const SEXP call, const SEXP op, const SEXP rho,
                   const SEXP retval);
void print_entry_info(const SEXP call, const SEXP op, const SEXP rho,
                      function_type fn_type);
void builtin_entry(const SEXP call, const SEXP op, const SEXP rho);
void specialsxp_entry(const SEXP call, const SEXP op, const SEXP rho);
void print_exit_info(const SEXP call, const SEXP op, const SEXP rho,
                     function_type fn_type);
void builtin_exit(const SEXP call, const SEXP op, const SEXP rho,
                  const SEXP retval);
void specialsxp_exit(const SEXP call, const SEXP op, const SEXP rho,
                     const SEXP retval);
void promise_created(const SEXP prom, const SEXP rho);
void force_promise_entry(const SEXP symbol, const SEXP rho);
void force_promise_exit(const SEXP symbol, const SEXP rho, const SEXP val);
void promise_lookup(const SEXP symbol, const SEXP rho, const SEXP val);
void promise_expression_lookup(const SEXP prom, const SEXP rho);
void gc_promise_unmarked(const SEXP promise);
void gc_entry(R_size_t size_needed);
void gc_exit(int gc_count, double vcells, double ncells);
void vector_alloc(int sexptype, long length, long bytes, const char *srcref);
void jump_ctxt(const SEXP rho, const SEXP val);

#endif /* __HOOKS_HPP__ */
