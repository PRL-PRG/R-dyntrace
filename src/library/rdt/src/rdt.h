#ifndef	_RDT_H
#define	_RDT_H

#include <rdtrace.h>

SEXP Rdt(SEXP tracer, SEXP rho, SEXP options);
SEXP Rdt_deparse(SEXP call);

rdt_handler *setup_default_tracing(SEXP options);
rdt_handler *setup_noop_tracing(SEXP options);
rdt_handler *setup_promise_tracing(SEXP options);

const char *get_string(SEXP sexp);

#endif /* _RDT_H */