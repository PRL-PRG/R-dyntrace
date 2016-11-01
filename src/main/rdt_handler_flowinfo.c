#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <rdtrace_probes.h>

#define OUT_FILENAME "flowinfo.out"
#define INDENT_LEVEL 2

static FILE *output = NULL;
static uint64_t last = 0;
static uint64_t delta = 0;
static unsigned int depth = 0;

static void print(const char *type, const char *fmt, ...) {
    char *entry = NULL;
    va_list ap;

    va_start(ap, fmt);
    if (!vasprintf(&entry, fmt, ap)) {
        perror("vasprintf()");
        return;
    }
    va_end(ap);

	fprintf(output, 
            "%12"PRId64" %-11s %*s%s\n", 
            delta,
            type, 
            depth * INDENT_LEVEL, "", 
            entry);

    free(entry);
}

static void compute_delta() {
    delta = (timestamp() - last) / 1000;
}

void probe_begin() {
    output = fopen(OUT_FILENAME, "wt");
    if (!output) {
        perror("fopen()");
    }

    fprintf(output, "%12s %-11s -- %s\n", "DELTA(us)", "TYPE", "NAME");
    last = timestamp();
}

void probe_end() {
    if (output) {
        fclose(output);
    }
}

void probe_function_entry(const SEXP call, const SEXP op, const SEXP rho) {
    compute_delta();

    const char *type = is_byte_compiled(call) ? "bc-function" : "function";
    const char *name = get_name(call);
    const char *ns = get_ns_name(op);
    char *loc = get_location(op);

    print(type, "-> %s::%s (loc: %s)", CHKSTR(ns), CHKSTR(name), CHKSTR(loc));

    if (loc) free(loc);
	
    depth++;
    last = timestamp();
}

void probe_function_exit(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
    compute_delta();
    depth -= depth > 0 ? 1 : 0;
    
    const char *type = is_byte_compiled(call) ? "bc-function" : "function";
    const char *name = get_name(call);
    const char *ns = get_ns_name(op);
    char *loc = get_location(op);
    char *result = to_string(retval);
    
    print(type, "<- %s::%s = `%s` (%d) (loc: %s)", CHKSTR(ns), CHKSTR(name), result, TYPEOF(retval), CHKSTR(loc));

    if (loc) free(loc);
    free(result);

    last = timestamp();
}

void probe_builtin_entry(const SEXP call) {
    compute_delta();

    const char *name = get_name(call);

    print("builtin", "-> %s", name);

	depth++;
    last = timestamp();
}

void probe_builtin_exit(const SEXP call, const SEXP retval) {
    compute_delta();
    depth -= depth > 0 ? 1 : 0;

    const char *name = get_name(call);
    char *result = to_string(retval);
    
    print("builtin", "<- %s = `%s` (%d)", CHKSTR(name), result, TYPEOF(retval));

    free(result);

    last = timestamp();
}

void probe_force_promise_entry(const SEXP symbol) {
    compute_delta();

    const char *name = get_name(symbol);
    
    print("promise", "-> %s", CHKSTR(name));

	depth++;
    last = timestamp();
}

void probe_force_promise_exit(const SEXP symbol, const SEXP val) {
    compute_delta();
    depth -= depth > 0 ? 1 : 0;

    const char *name = get_name(symbol);
    char *result = to_string(val);
    
    print("promise", "<- %s = `%s` (%d)", CHKSTR(name), result, TYPEOF(val));

    free(result);

    last = timestamp();
}

void probe_promise_lookup(const SEXP symbol, const SEXP val) {
    compute_delta();

    const char *name = get_name(symbol);
    char *result = to_string(val);
    
    print("lookup", "-- %s = `%s` (%d)", CHKSTR(name), result, TYPEOF(val));

    free(result);
    
    last = timestamp();
}

void probe_error(const SEXP call, const char* message) {
    compute_delta();

    const char *name = get_call(call);
    char *loc = get_location(call);
    
    print("lookup", "-- error in %s : %s (loc: %s)", CHKSTR(name), message, CHKSTR(loc));

    if (loc) free(loc);
    
    delta = 0;
    last = timestamp();
}