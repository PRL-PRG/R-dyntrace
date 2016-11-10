#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <rdtrace.h>

#include "rdt_trace.h"

static FILE *output = NULL;
static uint64_t last = 0;
static uint64_t delta = 0;
static unsigned int depth = 0;

static inline void print(const char *type, const char *loc, const char *name) {
	fprintf(output, 
            "%"PRId64",%d,%s,%s,%s\n", 
            delta,
            depth,
            type, 
            CHKSTR(loc),
            CHKSTR(name));
}

static inline void compute_delta() {
    delta = (timestamp() - last) / 1000;
}

void trace_begin() {
    fprintf(output, "DELTA,DEPTH,TYPE,LOCATION,NAME\n");
    fflush(output);

    last = timestamp();
}

void trace_function_entry(const SEXP call, const SEXP op, const SEXP rho) {
    compute_delta();

    const char *type = is_byte_compiled(call) ? "bc-function-entry" : "function-entry";
    const char *name = get_name(call);
    const char *ns = get_ns_name(op);
    char *loc = get_location(op);
    char *fqfn = NULL;

    if (ns) {
        asprintf(&fqfn, "%s::%s", ns, name);
    } else {
        fqfn = strdup(name);
    }

    print(type, loc, fqfn);

    if (loc) free(loc);
    if (fqfn) free(fqfn);
	
    depth++;
    last = timestamp();
}

void trace_function_exit(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
    compute_delta();
    depth -= depth > 0 ? 1 : 0;
    
    const char *type = is_byte_compiled(call) ? "bc-function-exit" : "function-exit";
    const char *name = get_name(call);
    const char *ns = get_ns_name(op);
    char *loc = get_location(op);
    char *fqfn = NULL;

    if (ns) {
        asprintf(&fqfn, "%s::%s", ns, name);
    } else {
        fqfn = strdup(name);
    }

    print(type, loc, fqfn);

    if (loc) free(loc);
    if (fqfn) free(fqfn);

    last = timestamp();
}

void trace_builtin_entry(const SEXP call) {
    compute_delta();

    const char *name = get_name(call);

    print("builtin-entry", NULL, name);

	depth++;
    last = timestamp();
}

void trace_builtin_exit(const SEXP call, const SEXP retval) {
    compute_delta();
    depth -= depth > 0 ? 1 : 0;

    const char *name = get_name(call);
    
    print("builtin-exit", NULL, name);

    last = timestamp();
}

void trace_force_promise_entry(const SEXP symbol) {
    compute_delta();

    const char *name = get_name(symbol);
    
    print("promise-entry", NULL, name);

	depth++;
    last = timestamp();
}

void trace_force_promise_exit(const SEXP symbol, const SEXP val) {
    compute_delta();
    depth -= depth > 0 ? 1 : 0;

    const char *name = get_name(symbol);

    print("promise-exit", NULL, name);

    last = timestamp();
}

void trace_promise_lookup(const SEXP symbol, const SEXP val) {
    compute_delta();

    const char *name = get_name(symbol);

    print("promise-lookup", NULL, name);
    
    last = timestamp();
}

void trace_error(const SEXP call, const char* message) {
    compute_delta();

    const char *name = get_call(call);
    char *loc = get_location(call);
    
    print("error", NULL, name);

    if (loc) free(loc);
    
    depth = 0;
    last = timestamp();
}

void trace_vector_alloc(int sexptype, long length, long bytes, const char* srcref) {
    compute_delta();
    print("vector-alloc", NULL, NULL);
    last = timestamp();
}

void trace_gc_entry(R_size_t size_needed) {
    compute_delta();
    print("builtin-entry", NULL, "gc_internal");
    last = timestamp();
}   

void trace_gc_exit(int gc_count, double vcells, double ncells) {
    compute_delta();
    print("builtin-exit", NULL, "gc_internal");
    last = timestamp();
}    

void trace_S3_generic_entry(const char *generic, const SEXP object) {
    compute_delta();
    
    print("s3-generic-entry", NULL, generic);
    
    depth++;
    last = timestamp();
}   

void trace_S3_generic_exit(const char *generic, const SEXP object, const SEXP retval) {
    compute_delta();
    depth -= depth > 0 ? 1 : 0;

    print("s3-generic-exit", NULL, generic);

    last = timestamp();
}

void trace_S3_dispatch_entry(const char *generic, const char *clazz, const SEXP method, const SEXP object) {
    compute_delta();
    
    print("s3-dispatch-entry", NULL, get_name(method));
    
    depth++;
    last = timestamp();
}

void trace_S3_dispatch_exit(const char *generic, const char *clazz, const SEXP method, const SEXP object, const SEXP retval) {
    compute_delta();
    
    depth -= depth > 0 ? 1 : 0;

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
    NULL, // probe_eval_entry
    NULL,  // probe_eval_exit
    &trace_gc_entry,        
    &trace_gc_exit,
    &trace_S3_generic_entry,
    &trace_S3_generic_exit,
    &trace_S3_dispatch_entry,        
    &trace_S3_dispatch_exit       
};

static int setup_tracing(const char *filename) {    
    output = filename != NULL ? fopen(filename, "wt") : stderr;
    if (!output) {
        error("Unable to open %s: %s\n", filename, strerror(errno));
        return 0;
    }

    return 1; 
}

static int running = 0;

SEXP RdtTrace(SEXP s_filename) {    
    const char *filename = s_filename != R_NilValue ? CHAR(STRING_ELT(s_filename, 0)) : NULL;

    if (running) {
        warning("RDT is already running");
        return R_TrueValue;
    }
    
    if (setup_tracing(filename)) { 
        rdt_start(&trace_rdt_handler);
        running = 1;
        return R_TrueValue;
    } else {
        error("Unable to initialize dynamic tracing");
        return R_FalseValue;
    }
}

SEXP RdtStop() {
    if (!running) {
        warning("RDT is not running\n");
        return R_FalseValue;
    }

    if(output != stderr) {
        fclose(output);
    }

    rdt_stop(&trace_rdt_handler);
    running = 0;
    
    return R_TrueValue;
}