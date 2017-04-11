#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cinttypes>
#include <string>

extern "C" {
#include "../../../main/inspect.h"
}

#include "rdt.h"

#include "rdt_register_hook.h"

static FILE *output = NULL;
static uint64_t last = 0;
static uint64_t delta = 0;

using namespace std;

static inline void print(string & type, string & loc, string & name) {
	fprintf(output, 
            "%" PRId64 ",%s,%s,%s\n",
            delta,
            type.c_str(),
            loc.c_str(),
            name.c_str());
}

static inline void compute_delta() {
    delta = (timestamp() - last) / 1000;
}

struct trace_debug {
    DECL_HOOK(begin)(const SEXP prom) {
        fprintf(output, "DELTA,TYPE,LOCATION,NAME\n");
        fflush(output);

        last = timestamp();
    }

    DECL_HOOK(function_entry)(const SEXP call, const SEXP op, const SEXP rho) {
        compute_delta();

        string type = is_byte_compiled(op) ? "bc-function-entry" : "function-entry";
        string name = CHKSTR(get_name(call));
        string loc = CHKSTR(get_location(op));

        const char * ns = get_ns_name(op);
        string fqfn = ns != NULL
                      ? (string(ns) + "::" + name)
                      : name;

        Rprintf("-------------------------------------------------------------------------------\n");
        print(type, loc, fqfn);
        Rprintf("call:\n");
        R_inspect(call);
        Rprintf("op:\n");
        R_inspect(op);
        Rprintf("rho:\n");
        R_inspect(rho);

        last = timestamp();
    }

    DECL_HOOK(function_exit)(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
        compute_delta();

        string type = is_byte_compiled(op) ? "bc-function-exit" : "function-exit";
        string name = CHKSTR(get_name(call));
        string loc = CHKSTR(get_location(op));

        const char * ns = get_ns_name(op);
        string fqfn = ns != NULL
                      ? (string(ns) + "::" + name)
                      : name;

        Rprintf("-------------------------------------------------------------------------------\n");
        print(type, loc, fqfn);
        Rprintf("call:\n");
        R_inspect(call);
        Rprintf("op:\n");
        R_inspect(op);
        Rprintf("rho:\n");
        R_inspect(rho);
        Rprintf("retval:\n");
        R_inspect(retval);

        last = timestamp();
    }

    DECL_HOOK(builtin_entry)(const SEXP call, const SEXP op, const SEXP rho) {
        compute_delta();

        string type = "builtin-entry";
        string name = CHKSTR(get_name(call));
        string loc = CHKSTR(get_location(op));

        const char * ns = get_ns_name(op);
        string fqfn = ns != NULL
                      ? (string(ns) + "::" + name)
                      : name;

        Rprintf("-------------------------------------------------------------------------------\n");
        print(type, loc, fqfn);
        Rprintf("call:\n");
        R_inspect(call);
        Rprintf("op:\n");
        R_inspect(op);
        Rprintf("rho:\n");
        R_inspect(rho);

        last = timestamp();
    }

    DECL_HOOK(builtin_exit)(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
        compute_delta();

        string type = "builtin-exit";
        string name = CHKSTR(get_name(call));
        string loc = CHKSTR(get_location(op));

        const char * ns = get_ns_name(op);
        string fqfn = ns != NULL
                      ? (string(ns) + "::" + name)
                      : name;

        Rprintf("-------------------------------------------------------------------------------\n");
        print(type, loc, fqfn);
        Rprintf("call:\n");
        R_inspect(call);
        Rprintf("op:\n");
        R_inspect(op);
        Rprintf("rho:\n");
        R_inspect(rho);
        Rprintf("retval:\n");
        R_inspect(retval);

        last = timestamp();
    }

    DECL_HOOK(specialsxp_entry)(const SEXP call, const SEXP op, const SEXP rho) {
        compute_delta();

        string type = "special-entry";
        string name = CHKSTR(get_name(call));
        string loc = CHKSTR(get_location(op));

        const char * ns = get_ns_name(op);
        string fqfn = ns != NULL
                      ? (string(ns) + "::" + name)
                      : name;

        Rprintf("-------------------------------------------------------------------------------\n");
        print(type, loc, fqfn);
        Rprintf("call:\n");
        R_inspect(call);
        Rprintf("op:\n");
        R_inspect(op);
        Rprintf("rho:\n");
        R_inspect(rho);

        last = timestamp();
    }

    DECL_HOOK(specialsxp_exit)(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
        compute_delta();

        string type = "special-exit";
        string name = CHKSTR(get_name(call));
        string loc = CHKSTR(get_location(op));

        const char * ns = get_ns_name(op);
        string fqfn = ns != NULL
                      ? (string(ns) + "::" + name)
                      : name;

        Rprintf("-------------------------------------------------------------------------------\n");
        print(type, loc, fqfn);
        Rprintf("call:\n");
        R_inspect(call);
        Rprintf("op:\n");
        R_inspect(op);
        Rprintf("rho:\n");
        R_inspect(rho);
        Rprintf("retval:\n");
        R_inspect(retval);

        last = timestamp();
    }

    DECL_HOOK(force_promise_entry)(const SEXP symbol, const SEXP rho) {
        compute_delta();

        string name = CHKSTR(get_name(symbol));
        string type = "promise-force-entry";
        string location = "<unknown>";

        Rprintf("-------------------------------------------------------------------------------\n");
        print(type, location, name);
        Rprintf("symbol:\n");
        R_inspect(symbol);
        Rprintf("rho:\n");
        R_inspect(rho);

        last = timestamp();
    }

    DECL_HOOK(force_promise_exit)(const SEXP symbol, const SEXP rho, const SEXP val) {
        compute_delta();

        string name = CHKSTR(get_name(symbol));
        string type = "promise-force-exit";
        string location = "<unknown>";

        Rprintf("-------------------------------------------------------------------------------\n");
        print(type, location, name);
        Rprintf("symbol:\n");
        R_inspect(symbol);
        Rprintf("rho:\n");
        R_inspect(rho);
        Rprintf("val:\n");
        R_inspect(val);

        last = timestamp();
    }

    DECL_HOOK(promise_lookup)(const SEXP symbol, const SEXP rho, const SEXP val) {
        compute_delta();

        string name = CHKSTR(get_name(symbol));
        string type = "promise-lookup";
        string location = "<unknown>";

        Rprintf("-------------------------------------------------------------------------------\n");
        print(type, location, name);
        Rprintf("symbol:\n");
        R_inspect(symbol);
        Rprintf("rho:\n");
        R_inspect(rho);
        Rprintf("val:\n");
        R_inspect(val);

        last = timestamp();
    }

    DECL_HOOK(error)(const SEXP call, const char* message) {
        compute_delta();

        string call_str = string("\"") + CHKSTR(get_call(call)) + string("\"");
        string loc = CHKSTR(get_location(call));
        string type = "error";

        print(type, loc, call_str);

        last = timestamp();
    }

    DECL_HOOK(vector_alloc)(int sexptype, long length, long bytes, const char* srcref) {
        compute_delta();

        string name = "<unknown>";
        string loc = "<unknown>";
        string type = "vector-alloc";

        print(type, loc, name);
        last = timestamp();
    }

    DECL_HOOK(gc_entry)(R_size_t size_needed) {
        compute_delta();

        string name = "gc-internal";
        string loc = "<unknown>";
        string type = "builtin-entry";

        Rprintf("-------------------------------------------------------------------------------\n");
        print(type, loc, name);
        last = timestamp();
    }

    DECL_HOOK(gc_exit)(int gc_count, double vcells, double ncells) {
        compute_delta();

        string name = "gc-internal";
        string loc = "<unknown>";
        string type = "builtin-exit";

        Rprintf("-------------------------------------------------------------------------------\n");
        print(type, loc, name);
        last = timestamp();
    }

    DECL_HOOK(S3_generic_entry)(const char *generic, const SEXP object) {
        compute_delta();

        string name = CHKSTR(generic);
        string loc = "<unknown>";
        string type = "s3-generic-entry";

        Rprintf("-------------------------------------------------------------------------------\n");
        print(type, loc, name);
        Rprintf("object:\n");
        R_inspect(object);

        last = timestamp();
    }

    DECL_HOOK(S3_generic_exit)(const char *generic, const SEXP object, const SEXP retval) {
        compute_delta();

        string name = CHKSTR(generic);
        string loc = "<unknown>";
        string type = "s3-generic-exit";


        Rprintf("-------------------------------------------------------------------------------\n");
        print(type, loc, name);
        Rprintf("object:\n");
        R_inspect(object);
        Rprintf("retval:\n");
        R_inspect(retval);

        last = timestamp();
    }

    DECL_HOOK(S3_dispatch_entry)(const char *generic, const char *clazz, const SEXP method, const SEXP object) {
        compute_delta();

        string name = get_name(method);
        string loc = "<unknown>";
        string type = "s3-dispatch-entry";

        if(name.empty())
            name = "<unknown>";

        Rprintf("-------------------------------------------------------------------------------\n");
        print(type, loc, name);
        Rprintf("method:\n");
        R_inspect(method);
        Rprintf("object:\n");
        R_inspect(object);

        last = timestamp();
    }

    DECL_HOOK(S3_dispatch_exit)(const char *generic, const char *clazz, const SEXP method, const SEXP object, const SEXP retval) {
        compute_delta();

        string name = get_name(method);
        string loc = "<unknown>";
        string type = "s3-dispatch-exit";

        if(name.empty())
            name = "<unknown>";

        Rprintf("-------------------------------------------------------------------------------\n");
        print(type, loc, name);
        Rprintf("method:\n");
        R_inspect(method);
        Rprintf("object:\n");
        R_inspect(object);
        Rprintf("retval:\n");
        R_inspect(retval);

        last = timestamp();
    }
};

// static void debug_eval_entry(SEXP e, SEXP rho) {
//     switch(TYPEOF(e)) {
//         case LANGSXP:
//             fprintf(output, "%s\n");
//             PrintValue
//         break;
//     }
// }

// static void debug_eval_exit(SEXP e, SEXP rho, SEXP retval) {
//     printf("");
// }    


rdt_handler *setup_debug_tracing(SEXP options) {
    const char *filename = get_string(get_named_list_element(options, "filename"));
    output = filename != NULL ? fopen(filename, "wt") : stderr;

    if (!output) {
        Rf_error("Unable to open %s: %s\n", filename, strerror(errno));
        return NULL;
    }

    rdt_handler *h = (rdt_handler *)  malloc(sizeof(rdt_handler));
    //memcpy(h, &debug_rdt_handler, sizeof(rdt_handler));
    *h = REGISTER_HOOKS(trace_debug,
                        tr::begin,
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
    
    SEXP disabled_probes = get_named_list_element(options, "disabled.probes");
    if (disabled_probes != R_NilValue && TYPEOF(disabled_probes) == STRSXP) {
        for (int i=0; i<LENGTH(disabled_probes); i++) {
            const char *probe = CHAR(STRING_ELT(disabled_probes, i));

            if (!strcmp("function", probe)) {
                h->probe_function_entry = NULL;
                h->probe_function_exit = NULL;
            } else if (!strcmp("builtin", probe)) {
                h->probe_builtin_entry = NULL;
                h->probe_builtin_exit = NULL;
            } else if (!strcmp("promise", probe)) {
                h->probe_promise_lookup = NULL;
                h->probe_force_promise_entry = NULL;
                h->probe_force_promise_exit = NULL;
            } else if (!strcmp("vector", probe)) {
                h->probe_vector_alloc = NULL;
            } else if (!strcmp("gc", probe)) {
                h->probe_gc_entry = NULL;
                h->probe_gc_exit = NULL;
            } else if (!strcmp("S3", probe)) {
                h->probe_S3_dispatch_entry = NULL;
                h->probe_S3_dispatch_exit = NULL;
                h->probe_S3_generic_entry = NULL;
                h->probe_S3_generic_exit = NULL;
            } else {
                warning("Unknown probe `%s`\n", probe);
            }
        }
    }

    last = 0;
    delta = 0;

    return h;
}
