#ifndef	_RDTRACE_H
#define	_RDTRACE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <Defn.h>

#define R_DT_HANDLER "R_DT_HANDLER"

int rdt_probe_begin_enabled();
int rdt_probe_end_enabled();
int rdt_probe_function_entry_enabled();
int rdt_probe_function_exit_enabled();
int rdt_probe_builtin_entry_enabled();
int rdt_probe_builtin_exit_enabled();
int rdt_probe_force_promise_entry_enabled();
int rdt_probe_force_promise_exit_enabled();
int rdt_probe_promise_lookup_enabled();
int rdt_probe_error_enabled();
int rdt_probe_vector_alloc_enabled();
int rdt_probe_eval_entry_enabled();
int rdt_probe_eval_exit_enabled();

void rdt_probe_begin();
void rdt_probe_end();
void rdt_probe_function_entry(const SEXP call, const SEXP op, const SEXP rho);
void rdt_probe_function_exit(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval);
void rdt_probe_builtin_entry(const SEXP call);
void rdt_probe_builtin_exit(const SEXP call, const SEXP retval);
void rdt_probe_force_promise_entry(const SEXP symbol);
void rdt_probe_force_promise_exit(const SEXP symbol, const SEXP val);
void rdt_probe_promise_lookup(const SEXP symbol, const SEXP val);
void rdt_probe_error(const SEXP call, const char* message);
void rdt_probe_vector_alloc(int sexptype, long length, long bytes, const char* srcref);
void rdt_probe_eval_entry(SEXP e, SEXP rho);
void rdt_probe_eval_exit(SEXP e, SEXP rho, SEXP retval);

void rdt_start();
void rdt_stop();

#endif	/* _RDTRACE_H */
