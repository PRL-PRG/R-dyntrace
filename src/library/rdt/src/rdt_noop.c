#include <stdlib.h>
#include "rdt.h"

static void noop_begin() {
}

static void noop_function_entry(const SEXP call, const SEXP op, const SEXP rho) {
}

static void noop_function_exit(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
}

static void noop_builtin_entry(const SEXP call, const SEXP op, const SEXP rho) {
}

static void noop_builtin_exit(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
}

static void noop_force_promise_entry(const SEXP symbol, const SEXP rho) {
}

static void noop_force_promise_exit(const SEXP symbol, const SEXP val) {
}

static void noop_promise_lookup(const SEXP symbol, const SEXP val) {
}

static void noop_error(const SEXP call, const char* message) {
}

static void noop_vector_alloc(int sexptype, long length, long bytes, const char* srcref) {
}

// static void noop_eval_entry(SEXP e, SEXP rho) {
//     switch(TYPEOF(e)) {
//         case LANGSXP:
//             fprintf(output, "%s\n");
//             PrintValue
//         break;
//     }
// }

// static void noop_eval_exit(SEXP e, SEXP rho, SEXP retval) {
//     printf("");
// }    

static void noop_gc_entry(R_size_t size_needed) {
}   

static void noop_gc_exit(int gc_count, double vcells, double ncells) {
}    

static void noop_S3_generic_entry(const char *generic, const SEXP object) {
}   

static void noop_S3_generic_exit(const char *generic, const SEXP object, const SEXP retval) {
}

static void noop_S3_dispatch_entry(const char *generic, const char *clazz, const SEXP method, const SEXP object) {
}

static void noop_S3_dispatch_exit(const char *generic, const char *clazz, const SEXP method, const SEXP object, const SEXP retval) {
}

static const rdt_handler noop_rdt_handler = {
    &noop_begin,
    NULL,
    &noop_function_entry,
    &noop_function_exit,
    &noop_builtin_entry,
    &noop_builtin_exit,
    &noop_force_promise_entry,
    &noop_force_promise_exit,
    &noop_promise_lookup,
    &noop_error,
    &noop_vector_alloc,
    NULL,
    NULL,
    // &noop_eval_entry,
    // &noop_eval_exit,
    &noop_gc_entry,        
    &noop_gc_exit,
    &noop_S3_generic_entry,
    &noop_S3_generic_exit,
    &noop_S3_dispatch_entry,        
    &noop_S3_dispatch_exit       
};

rdt_handler *setup_noop_tracing(SEXP options) {
    rdt_handler *h = (rdt_handler *)  malloc(sizeof(rdt_handler));
    memcpy(h, &noop_rdt_handler, sizeof(rdt_handler));

    return h;
}