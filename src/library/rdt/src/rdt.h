#ifndef	_RDT_H
#define	_RDT_H

#ifdef __cplusplus
extern "C" {
#include "r.h"
#endif

#include <rdtrace.h>

SEXP Rdt(SEXP tracer, SEXP rho, SEXP options);

rdt_handler *setup_default_tracing(SEXP options);
rdt_handler *setup_noop_tracing(SEXP options);
rdt_handler *setup_promise_tracing(SEXP options);
rdt_handler *setup_debug_tracing(SEXP options);

void cleanup_promise_tracing(/* rdt_handler *handler */ SEXP options);

const char *get_string(SEXP sexp);

#ifdef __cplusplus
}
#endif

#endif /* _RDT_H */