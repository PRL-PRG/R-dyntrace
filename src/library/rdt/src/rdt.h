#ifndef _RDT_H
#define _RDT_H

#ifdef __cplusplus
#include "r.h"
#endif
#include <rdtrace.h>

#ifdef __cplusplus
extern "C" {
#endif

SEXP Rdt(SEXP tracer, SEXP library_filepath, SEXP rho,
         SEXP options);

typedef rdt_handler *(*tracer_setup_t)(SEXP);
typedef void (*tracer_cleanup_t)(SEXP);

const char *get_string(SEXP sexp);

#ifdef __cplusplus
}
#endif

#endif /* _RDT_H */
