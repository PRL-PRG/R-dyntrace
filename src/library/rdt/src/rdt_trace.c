#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "rdt.h"

static FILE *output = NULL;
static uint64_t last = 0;
static uint64_t delta = 0;

static inline void print(const char *type, const char *loc, const char *name) {
	fprintf(output, 
            "%"PRId64",%s,%s,%s\n", 
            delta,
            type, 
            CHKSTR(loc),
            CHKSTR(name));
}

static inline void compute_delta() {
    delta = (timestamp() - last) / 1000;
}

static void trace_begin() {
    fprintf(output, "DELTA,TYPE,LOCATION,NAME\n");
    fflush(output);

    last = timestamp();
}

static void trace_function_entry(const SEXP call, const SEXP op, const SEXP rho) {
    compute_delta();

    const char *type = is_byte_compiled(call) ? "bc-function-entry" : "function-entry";
    const char *name = get_name(call);
    const char *ns = get_ns_name(op);
    char *loc = get_location(op);
    char *fqfn = NULL;

    if (ns) {
        asprintf(&fqfn, "%s::%s", ns, CHKSTR(name));
    } else {
        fqfn = name != NULL ? strdup(name) : NULL;
    }

    print(type, loc, fqfn);

    if (loc) free(loc);
    if (fqfn) free(fqfn);
	
    last = timestamp();
}

static void trace_function_exit(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
    compute_delta();
    
    const char *type = is_byte_compiled(call) ? "bc-function-exit" : "function-exit";
    const char *name = get_name(call);
    const char *ns = get_ns_name(op);
    char *loc = get_location(op);
    char *fqfn = NULL;

    if (ns) {
        asprintf(&fqfn, "%s::%s", ns, CHKSTR(name));
    } else {
        fqfn = name != NULL ? strdup(name) : NULL;
    }

    print(type, loc, fqfn);

    if (loc) free(loc);
    if (fqfn) free(fqfn);

    last = timestamp();
}

static void trace_builtin_entry(const SEXP call, const SEXP op, const SEXP rho) {
    compute_delta();

    const char *name = get_name(call);

    print("builtin-entry", NULL, name);

    last = timestamp();
}

static void trace_builtin_exit(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
    compute_delta();

    const char *name = get_name(call);
    
    print("builtin-exit", NULL, name);

    last = timestamp();
}

static void trace_force_promise_entry(const SEXP symbol, const SEXP rho) {
    compute_delta();

    const char *name = get_name(symbol);
    
    print("promise-entry", NULL, name);

    last = timestamp();
}

static void trace_force_promise_exit(const SEXP symbol, const SEXP rho, const SEXP val) {
    compute_delta();

    const char *name = get_name(symbol);

    print("promise-exit", NULL, name);

    last = timestamp();
}

static void trace_promise_lookup(const SEXP symbol, const SEXP rho, const SEXP val) {
    compute_delta();

    const char *name = get_name(symbol);

    print("promise-lookup", NULL, name);
    
    last = timestamp();
}

static void trace_error(const SEXP call, const char* message) {
    compute_delta();

    char *call_str = NULL;
    char *loc = get_location(call);
    
    asprintf(&call_str, "\"%s\"", get_call(call));
    
    print("error", NULL, call_str);

    if (loc) free(loc);
    if (call_str) free(call_str);
    
    last = timestamp();
}

static void trace_vector_alloc(int sexptype, long length, long bytes, const char* srcref) {
    compute_delta();
    print("vector-alloc", NULL, NULL);
    last = timestamp();
}

// static void trace_eval_entry(SEXP e, SEXP rho) {
//     switch(TYPEOF(e)) {
//         case LANGSXP:
//             fprintf(output, "%s\n");
//             PrintValue
//         break;
//     }
// }

// static void trace_eval_exit(SEXP e, SEXP rho, SEXP retval) {
//     printf("");
// }    

static void trace_gc_entry(R_size_t size_needed) {
    compute_delta();
    print("builtin-entry", NULL, "gc_internal");
    last = timestamp();
}   

static void trace_gc_exit(int gc_count, double vcells, double ncells) {
    compute_delta();
    print("builtin-exit", NULL, "gc_internal");
    last = timestamp();
}    

static void trace_S3_generic_entry(const char *generic, const SEXP object) {
    compute_delta();
    
    print("s3-generic-entry", NULL, generic);
    
    last = timestamp();
}   

static void trace_S3_generic_exit(const char *generic, const SEXP object, const SEXP retval) {
    compute_delta();

    print("s3-generic-exit", NULL, generic);

    last = timestamp();
}

static void trace_S3_dispatch_entry(const char *generic, const char *clazz, const SEXP method, const SEXP object) {
    compute_delta();
    
    print("s3-dispatch-entry", NULL, get_name(method));
    
    last = timestamp();
}

static void trace_S3_dispatch_exit(const char *generic, const char *clazz, const SEXP method, const SEXP object, const SEXP retval) {
    compute_delta();
    
    print("s3-dispatch-exit", NULL, get_name(method));
    
    last = timestamp();
}

static const rdt_handler trace_rdt_handler = {
    &trace_begin,
    NULL,
    &trace_function_entry,
    &trace_function_exit,
    &trace_builtin_entry,
    &trace_builtin_exit,
    &trace_force_promise_entry,
    &trace_force_promise_exit,
    &trace_promise_lookup,
    &trace_error,
    &trace_vector_alloc,
    NULL, // &trace_eval_entry,
    NULL, // &trace_eval_exit,
    &trace_gc_entry,        
    &trace_gc_exit,
    NULL,
    NULL,
    &trace_S3_generic_entry,
    &trace_S3_generic_exit,
    &trace_S3_dispatch_entry,        
    &trace_S3_dispatch_exit       
};

rdt_handler *setup_default_tracing(SEXP options) {
    const char *filename = get_string(get_named_list_element(options, "filename"));
    output = filename != NULL ? fopen(filename, "wt") : stderr;

    if (!output) {
        error("Unable to open %s: %s\n", filename, strerror(errno));
        return NULL;
    }

    rdt_handler *h = (rdt_handler *)  malloc(sizeof(rdt_handler));
    memcpy(h, &trace_rdt_handler, sizeof(rdt_handler));
    
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
