#ifndef	_RDTRACE_H
#define	_RDTRACE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <Defn.h>

#define RDT_HANDLER_ENV_VAR "R_RDT_HANDLER"
#define RDT_IS_ENABLED(name) (rdt_##name##_ref != NULL)
#define RDT_FIRE_PROBE(name, ...) ((*rdt_##name##_ref)(__VA_ARGS__))

void rdt_start();
void rdt_stop();

//-----------------------------------------------------------------------------
// probes refererences
//-----------------------------------------------------------------------------

extern void (*rdt_probe_begin_ref)();
extern void (*rdt_probe_end_ref)();
extern void (*rdt_probe_function_entry_ref)(const SEXP call, const SEXP op, const SEXP rho);
extern void (*rdt_probe_function_exit_ref)(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval);
extern void (*rdt_probe_builtin_entry_ref)(const SEXP call);
extern void (*rdt_probe_builtin_exit_ref)(const SEXP call, const SEXP retval);
extern void (*rdt_probe_force_promise_entry_ref)(const SEXP symbol);
extern void (*rdt_probe_force_promise_exit_ref)(const SEXP symbol, const SEXP val);
extern void (*rdt_probe_promise_lookup_ref)(const SEXP symbol, const SEXP val);
extern void (*rdt_probe_error_ref)(const SEXP call, const char* message);
extern void (*rdt_probe_vector_alloc_ref)(int sexptype, long length, long bytes, const char* srcref);
extern void (*rdt_probe_eval_entry_ref)(SEXP e, SEXP rho);
extern void (*rdt_probe_eval_exit_ref)(SEXP e, SEXP rho, SEXP retval);

#endif	/* _RDTRACE_H */
