#include <cstdlib>
#include <cstring>

#include "rdt.h"
#include "rdt_register_hook.h"

struct trace_noop {
    DECL_HOOK(begin)(const SEXP prom) {
    }

    DECL_HOOK(function_entry)(const SEXP call, const SEXP op, const SEXP rho) {
    }

    DECL_HOOK(function_exit)(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
    }

    DECL_HOOK(builtin_entry)(const SEXP call, const SEXP op, const SEXP rho) {
    }

    DECL_HOOK(builtin_exit)(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
    }

    DECL_HOOK(force_promise_entry)(const SEXP symbol, const SEXP rho) {
    }

    DECL_HOOK(force_promise_exit)(const SEXP symbol, const SEXP rho, const SEXP val) {
    }

    DECL_HOOK(promise_lookup)(const SEXP symbol, const SEXP rho, const SEXP val) {
    }

    DECL_HOOK(error)(const SEXP call, const char* message) {
    }

    DECL_HOOK(vector_alloc)(int sexptype, long length, long bytes, const char* srcref) {
    }

    DECL_HOOK(gc_entry)(R_size_t size_needed) {
    }

    DECL_HOOK(gc_exit)(int gc_count, double vcells, double ncells) {
    }

    DECL_HOOK(S3_generic_entry)(const char *generic, const SEXP object) {
    }

    DECL_HOOK(S3_generic_exit)(const char *generic, const SEXP object, const SEXP retval) {
    }

    DECL_HOOK(S3_dispatch_entry)(const char *generic, const char *clazz, const SEXP method, const SEXP object) {
    }

    DECL_HOOK(S3_dispatch_exit)(const char *generic, const char *clazz, const SEXP method, const SEXP object, const SEXP retval) {
    }
};

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

rdt_handler *setup_noop_tracing(SEXP options) {
    rdt_handler *h = (rdt_handler *)  malloc(sizeof(rdt_handler));
    //memcpy(h, &noop_rdt_handler, sizeof(rdt_handler));
    *h = REGISTER_HOOKS(trace_noop,
                        tr::begin,
                        tr::function_entry,
                        tr::function_exit,
                        tr::builtin_entry,
                        tr::builtin_exit,
                        tr::force_promise_entry,
                        tr::force_promise_exit,
                        tr::promise_lookup,
                        tr::error,
                        tr::vector_alloc,
                        tr::gc_entry,
                        tr::gc_exit,
                        tr::S3_generic_entry,
                        tr::S3_generic_exit,
                        tr::S3_dispatch_entry,
                        tr::S3_dispatch_exit);

    return h;
}