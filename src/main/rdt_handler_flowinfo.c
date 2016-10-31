#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <rdtrace_probes.h>

#define OUT_FILENAME "flowinfo.out"

static FILE *output = NULL;
static unsigned long last = 0;
static unsigned int depth = 0;

void probe_begin() {
    output = fopen(OUT_FILENAME, "wt");
    if (!output) {
        perror("fopen()");
    }

    fprintf(output, "%12s %5s %-8s -- %s\n", "DELTA(us)", "FLAGS", "TYPE", "NAME");
    last = timestamp();
}

void probe_end() {
    if (output) {
        fclose(output);
    }
}

void probe_function_entry(const SEXP call, const SEXP op, const SEXP rho) {
    unsigned long delta = (timestamp() - last) / 1000;
    char *name = get_fun_fq_name(call, op);
    char *loc = get_location(op);

    fprintf(output, 
            "%12lu %5d %-8s %*s-> %s (loc: %s)\n", 
            delta, 
            is_byte_compiled(call),
            "function", 
            depth * 2, "",
            name, 
	        loc);

	depth++;
    last = timestamp();

    free(name);
    free(loc);
}

void probe_function_exit(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
    unsigned long delta = (timestamp() - last) / 1000;
    char *name = get_fun_fq_name(call, op);
    char *loc = get_location(op);
    char *result = to_string(retval);

    depth -= depth > 0 ? 1 : 0;
    last = timestamp();

	fprintf(output, 
            "%12lu %5d %-8s %*s<- %s = `%s` (%d) (loc: %s)\n", 
            delta, 
            is_byte_compiled(call),
            "function", 
            depth * 2, "",
	        name,
            result,
            TYPEOF(retval),
            loc);

    free(name);
    free(loc);
    free(result);
}
