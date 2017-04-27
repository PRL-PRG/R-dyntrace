#ifndef	_RDT_H
#define	_RDT_H

#ifdef __cplusplus
#include "r.h"
#endif
#include <rdtrace.h>

#ifdef __cplusplus
extern "C" {
#endif

SEXP Rdt(SEXP tracer, SEXP rho, SEXP options);

typedef rdt_handler * (*tracer_setup_ptr_t)(SEXP);
typedef void (*tracer_cleanup_ptr_t)(SEXP);

rdt_handler *setup_default_tracing(SEXP options);
rdt_handler *setup_noop_tracing(SEXP options);
rdt_handler *setup_debug_tracing(SEXP options);
rdt_handler *setup_specialsxp_tracing(SEXP options);

void cleanup_promises_tracing(/* rdt_handler *handler */ SEXP options);

__attribute__((weak)) const char *get_string(SEXP sexp) {
    if (sexp == R_NilValue || TYPEOF(sexp) != STRSXP) {
        return NULL;
    }

    return CHAR(STRING_ELT(sexp, 0));
}

#ifdef __cplusplus
}
#endif

#endif /* _RDT_H */