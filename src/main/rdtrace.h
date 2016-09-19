#ifndef	_RDTRACE_H
#define	_RDTRACE_H

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Defn.h>
#include <rdtrace_probes.h>

#define FUN_NO_FLAGS 0x0
#define FUN_BC 0x1

void rdtrace_function_entry(SEXP call, SEXP op, SEXP rho);
void rdtrace_function_exit(SEXP call, SEXP op, SEXP rho, SEXP rv);
void rdtrace_force_promise_entry(SEXP symbol);
void rdtrace_force_promise_exit(SEXP symbol, SEXP val);
void rdtrace_promise_lookup(SEXP symbol, SEXP val);

#endif	/* _RDTRACE_H */
