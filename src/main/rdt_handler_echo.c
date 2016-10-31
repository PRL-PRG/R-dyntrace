#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <rdtrace_probes.h>

#define OUT_FILENAME_VAR "R_RDT_ECHO_HANDLER_OUT"

static FILE *output = NULL;

static void echo(const char *fmt, ...) {
    char *res = NULL;
    if (!asprintf(&res, "RDT: %s\n", fmt)) {
        perror("asprintf()");
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    
    vfprintf(output, res, ap);
    
    va_end(ap);
    free(res);
}

void probe_begin() {
    char *fname = getenv(OUT_FILENAME_VAR);
    if (!fname || strlen(fname) == 0) {
        output = stderr;
    } else {
        output = fopen(fname, "wt");
        if (!output) {
            char *msg;
            
            asprintf(&msg, "Unable to open %s\n", fname);
            perror(msg);
            free(msg);

            output = stderr;
        }
    } 

    echo("begin(%s)", fname);
}

void probe_end() {
    echo("end()");
    fclose(output);
}

void probe_function_entry(const SEXP call, const SEXP op, const SEXP rho) {
    char *name = get_fun_fq_name(call, op);
    char *loc = get_location(op);

    echo("function-entry(%s, %s, %d)", name, loc, is_byte_compiled(op));

    free(name);
    free(loc);
}

void probe_function_exit(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
    char *name = get_fun_fq_name(call, op);
    char *loc = get_location(op);
    char *result = to_string(retval);
     
    echo("function-exit(%s, %s, %d, `%s`)", name, loc, is_byte_compiled(op), result);

    free(name);
    free(loc);
    free(result);
} 

void probe_builtin_entry(const SEXP call) {
    echo("builtin-entry(%s)", get_fun_name(call));
}

void probe_builtin_exit(const SEXP call, const SEXP retval) {
    char *result = to_string(retval);

    echo("builtin-exit(%s, %s)", get_fun_name(call), result);

    free(result);
}

void probe_force_promise_entry(const SEXP symbol) {
        echo("force-promise-entry(%s)", get_symbol_name(symbol));
}

void probe_force_promise_exit(const SEXP symbol, const SEXP val) {
    char *result = to_string(val);

    echo("force-promise-exit(%s, `%s`)", get_symbol_name(symbol), result);

    free(result);
}

void probe_promise_lookup(const SEXP symbol, const SEXP val) {
    char *result = to_string(val);

    echo("promise-lookup(%s, `%s`)", get_symbol_name(symbol), result);

    free(result);
}

void probe_error(const SEXP call, const char* message) {
    char *loc = get_location(call);

    echo("error(%s, %s, %s)", get_call(call), loc, message);

    free(loc);    
} 

void probe_vector_alloc(int sexptype, long length, long bytes, const char* srcref) {
    echo("vector-alloc(%d, %ld, %ld, %s)", sexptype, length, bytes, srcref);
}

void probe_eval_entry(SEXP e, SEXP rho) {
    // echo("eval-entry(%s)", get_expression(e));
    echo("eval-entry(%s)", "");
}

void probe_eval_exit(SEXP e, SEXP rho, SEXP retval) {
    char *result = to_string(retval);

    // echo("eval-exit(%s, `%s`)", get_expression(e), result);
    echo("eval-exit(%s, `%s`)", "", "");

    free(result);    
}