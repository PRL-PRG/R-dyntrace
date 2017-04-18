#include <cstdlib>
#include <cstring>

#include "rdt.h"


struct trace_noop {
    static void begin(const SEXP prom) {
    }

    static void function_entry(const SEXP call, const SEXP op, const SEXP rho) {
    }

    static void function_exit(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
    }

    static void builtin_entry(const SEXP call, const SEXP op, const SEXP rho) {
    }

    static void builtin_exit(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
    }

    static void specialsxp_entry(const SEXP call, const SEXP op, const SEXP rho) {
    }

    static void specialsxp_exit(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
    }

    static void force_promise_entry(const SEXP symbol, const SEXP rho) {
    }

    static void force_promise_exit(const SEXP symbol, const SEXP rho, const SEXP val) {
    }

    static void promise_lookup(const SEXP symbol, const SEXP rho, const SEXP val) {
    }

    static void error(const SEXP call, const char* message) {
    }

    static void vector_alloc(int sexptype, long length, long bytes, const char* srcref) {
    }

    static void gc_entry(R_size_t size_needed) {
    }

    static void gc_exit(int gc_count, double vcells, double ncells) {
    }

    static void S3_generic_entry(const char *generic, const SEXP object) {
    }

    static void S3_generic_exit(const char *generic, const SEXP object, const SEXP retval) {
    }

    static void S3_dispatch_entry(const char *generic, const char *clazz, const SEXP method, const SEXP object) {
    }

    static void S3_dispatch_exit(const char *generic, const char *clazz, const SEXP method, const SEXP object, const SEXP retval) {
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

    REG_HOOKS_BEGIN(h, trace_noop);
        ADD_HOOK(begin);
        ADD_HOOK(function_entry);
        ADD_HOOK(function_exit);
        ADD_HOOK(builtin_entry);
        ADD_HOOK(builtin_exit);
        ADD_HOOK(specialsxp_entry);
        ADD_HOOK(specialsxp_exit);
        ADD_HOOK(force_promise_entry);
        ADD_HOOK(force_promise_exit);
        ADD_HOOK(promise_lookup);
        ADD_HOOK(error);
        ADD_HOOK(vector_alloc);
        ADD_HOOK(gc_entry);
        ADD_HOOK(gc_exit);
        ADD_HOOK(S3_generic_entry);
        ADD_HOOK(S3_generic_exit);
        ADD_HOOK(S3_dispatch_entry);
        ADD_HOOK(S3_dispatch_exit);
    REG_HOOKS_END;

    return h;
}