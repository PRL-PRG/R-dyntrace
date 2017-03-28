#include <stdlib.h>
#include <map>
#include <sstream>

extern "C" {
// If I don't include this before rdt.h, I get strange compiler errors...
#include "r.h"
#include "rdt.h"
}

#include "rdt_register_hook.h"

#include "rdt_promises/tracer_conf.h"
#include "rdt_promises/tracer_output.h"

static std::map<std::string, uint64_t> specialsxp_count;

struct trace_specialsxp {

    DECL_HOOK(begin)(const SEXP prom) {
    }

    DECL_HOOK(end)() {
        for (auto &pair : specialsxp_count) {
            Rprintf("%s : %llu\n", pair.first.c_str(), pair.second);
        }
    }

    DECL_HOOK(function_entry)(const SEXP call, const SEXP op, const SEXP rho) {
    }

    DECL_HOOK(function_exit)(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
    }

    DECL_HOOK(builtin_entry)(const SEXP call, const SEXP op, const SEXP rho) {
    }

    DECL_HOOK(builtin_exit)(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
    }

    DECL_HOOK(specialsxp_entry)(const SEXP call, const SEXP op, const SEXP rho) {

        std::string call_name = std::string(get_name(call));

        if (specialsxp_count.find(call_name) == specialsxp_count.end()) {
            specialsxp_count[call_name] = 1;
        } else {
            specialsxp_count[call_name]++;
        }
    }

    DECL_HOOK(specialsxp_exit)(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
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


rdt_handler *setup_specialsxp_tracing(SEXP options) {
    rdt_handler *h = (rdt_handler *)  malloc(sizeof(rdt_handler));
    *h = REGISTER_HOOKS(trace_specialsxp,
                        tr::begin,
                        tr::end,
                        tr::function_entry,
                        tr::function_exit,
                        tr::builtin_entry,
                        tr::builtin_exit,
                        tr::specialsxp_entry,
                        tr::specialsxp_exit,
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