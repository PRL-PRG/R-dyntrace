#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
//#include <Defn.h>
#include "../../../main/inspect.h"

#include "rdt.h"

#define RDT_PROMISES_INDENT

static FILE *output = NULL;
static uint64_t last = 0;
static uint64_t delta = 0;

static int indent;

static inline char *mk_indent() {
    int indent_length = indent * 4;
    char *indent_string = malloc(sizeof(char) * (indent_length + 1));
    for (int i = 0; i<indent_length; i++)
        indent_string[i] = ' ';
    indent_string[indent_length] = '\0';
    return indent_string;
}

// XXX probably remove
static inline void p_print(const char *type, const char *loc, const char *name) {
    #ifdef RDT_PROMISES_INDENT
    char *indent_string = mk_indent();

    fprintf(output,
            "%s%"PRId64",%s,%s,%s\n",
            indent_string,
            delta,
            type,
            CHKSTR(loc),
            CHKSTR(name));

    if (indent_string)
        free(indent_string);
    #else
    printf(output,
            "%s%"PRId64",%s,%s,%s\n",
            indent_string,
            delta,
            type,
            CHKSTR(loc),
            CHKSTR(name));
    #endif
}

static inline char *concat_arguments(const char *arguments[], const int arguments_length, const char* separator) {
    int characters = 0;
    for (int i = 0; i<arguments_length; i++) {
        characters += strlen(arguments[i]); /* for the argument name */
        //if (default_values[i] != NULL)
        //    characters += strlen(default_values[i]) /* for the expression */ + 1 /* for "=" */;
    }

    char *argument_string = calloc(1, sizeof(char) * (characters + strlen(separator) * (arguments_length - 1)/* commas */ + 1 /* terminator */));

    for (int i=0; i<arguments_length; i++) {
        if (i)
            argument_string = strcat(argument_string, separator);
        argument_string = strcat(argument_string, arguments[i]);

        //if (default_values[i] != NULL) {
        //  argument_string = strcat(argument_string, "=");
        //  argument_string = strcat(argument_string, default_values[i]);
        //}
    }

    return argument_string;
}

// TODO ifdefs
static inline void print_function(const char *type, const char *loc, const char *name, const char **arguments, const char **default_values, const int arguments_num) {

    char *indent_string = mk_indent();
    char *argument_string = concat_arguments(arguments, arguments_num, /*separator=*/ ",");

    fprintf(output,
        "%s%s @%s %s(%s)\n",
        indent_string,
        type,
        CHKSTR(loc),
        CHKSTR(name),
        argument_string
    );

    if (indent_string)
        free(indent_string);
    if (argument_string)
        free(argument_string);
};

// TODO remove
static inline void compute_delta() {
    delta = (timestamp() - last) / 1000;
}

// ??? can we get metadata about the program we're analysing in here?
static void trace_promises_begin() {
    indent = 0;

    //fprintf(output, "TYPE,LOCATION,NAME\n");
    //fflush(output);

    last = timestamp();
}

static inline int count_elements(SEXP list) {
    int counter = 0;
    SEXP tmp = list;
    for (; tmp != R_NilValue; counter++)
        tmp = CDR(tmp);
    return counter;
}

static inline char *trim_string(char *str) {
    if (str == NULL)
        return str;

    int offset_bow = 0, offset_aft = 0;
    char *aft = str + strlen(str) - 1;

    for (; isspace((unsigned char) *(str + offset_bow)); offset_bow++);
    for (; isspace((unsigned char) *(aft + offset_aft)); offset_aft--);

    char *ret = malloc(sizeof(char *) * offset_aft - offset_bow + 1);
    for (int si = offset_bow, ri = 0; si < offset_aft; si++, ri++)
        ret[ri] = str[si];

    return ret;
}

static inline char **strings_of_STRSXP(SEXP str, Rboolean flatten) {
    // Currently I just want to handle a specific case here, so I'll return NULL for everything else.
    if (TYPEOF(str) != STRSXP)
        return NULL;

    int size = count_elements(str);

    char **strings = malloc((sizeof(char *) * size));
    for (int i = 0; i < size; str=CDR(str), i++) {
        Rprintf(">-----------------------[%d]", i);

        strings[i] = strdup(CHAR(str));

        Rprintf("<-----------------------[%d] %s", i, strings[i]);
    }
}

static inline int get_arguments(SEXP op, SEXP rho, char ***return_arguments, char ***return_default_values) {
    SEXP formals = FORMALS(op);

    int argument_count = count_elements(formals);

    char **arguments = malloc((sizeof(char *) * argument_count));
    char **argument_default_values = malloc ((sizeof(char *) * argument_count));
    for (int i=0; i<argument_count; i++) {
        arguments[i] = strdup(get_name(TAG(formals)));

        // XXX dot-dot-dot and other weird cases?

        SEXP car = CAR(formals);
        if (car != R_MissingArg) { // TODO get string of entire expression
            argument_default_values[i] = NULL;

            SEXP deparse = deparse1(car, FALSE, 0);
            R_inspect(deparse);

            //argument_default_values[i] = strdup();
        } else
            argument_default_values[i] = NULL;

        // The call SEXP only contains AST to find the actual argument value, we need to search the environment
        SEXP value = findVar(TAG(formals), rho);
        R_inspect(value);

        formals = CDR(formals);
    }

    (*return_arguments) = arguments;
    (*return_default_values) = argument_default_values;
    return argument_count;
}

// Triggggerrredd when entering function evaluation.
// TODO: function name, unique function identifier, arguments and their order, promises
// ??? where are promises created? and do we care?
// ??? will address of funciton change? garbage collector?
static void trace_promises_function_entry(const SEXP call, const SEXP op, const SEXP rho) {
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

    char **arguments;
    char **default_values;
    int argument_count;

    argument_count = get_arguments(op, rho, &arguments, &default_values);

    //printf("INSPECT ARGS:\n");


    print_function(type, loc, name, arguments, default_values, argument_count);

    printf("-------------------------\n");
    //R_inspect(call);
    printf("-------------------------\n");
    //R_inspect(op);
    printf("-------------------------\n");


    #ifdef RDT_PROMISES_INDENT
    indent++;
    #endif

    if (arguments) free(arguments);
    if (default_values) free(default_values);


    last = timestamp();
}

static void trace_promises_function_exit(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
    compute_delta();

    #ifdef RDT_PROMISES_INDENT
    indent--;
    #endif

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

    p_print(type, loc, fqfn);

    if (loc) free(loc);
    if (fqfn) free(fqfn);

    last = timestamp();
}

// XXX Probably don't need this?
static void trace_promises_builtin_entry(const SEXP call) {
    compute_delta();

    const char *name = get_name(call);

    p_print("builtin-entry", NULL, name);

    last = timestamp();
}

static void trace_promises_builtin_exit(const SEXP call, const SEXP retval) {
    compute_delta();

    const char *name = get_name(call);

    p_print("builtin-exit", NULL, name);

    last = timestamp();
}

// Promise is being used inside a function body for the first time.
// TODO name of promise, expression inside promise, value evaluated if available, (in the long term) connected to a function
// TODO get more info to hook from eval (eval.c::4401 and at least 1 more line)
static void trace_promises_force_promise_entry(const SEXP symbol, const SEXP rho) {
    compute_delta();

    const char *name = get_name(symbol);

    p_print("promise-entry", NULL, name);

    if (TYPEOF(symbol) == PROMSXP) {
        R_inspect(symbol);
    }
    else if (TYPEOF(symbol) == SYMSXP) {
        SEXP value = findVar(symbol, rho);
        R_inspect(value);
    }


    //R_inspect(symbol);
    //printf("\n");

    //R_inspect(SYMVALUE(symbol));
    //printf("\n");

//    R_inspect(INTERNAL(symbol));
//    printf("\n");


    last = timestamp();
}

static void trace_promises_force_promise_exit(const SEXP symbol, const SEXP val) {
    compute_delta();

    const char *name = get_name(symbol);

    p_print("promise-exit", NULL, name);

    //R_inspect(symbol);
    //printf("\n");

    //R_inspect(val);
    //printf("\n");

    last = timestamp();
}

static void trace_promises_promise_lookup(const SEXP symbol, const SEXP val) {
    compute_delta();

    const char *name = get_name(symbol);

    p_print("promise-lookup", NULL, name);

    //R_inspect(symbol);
    //printf("\n");

    //R_inspect(val);
    //printf("\n");

    last = timestamp();
}

static void trace_promises_error(const SEXP call, const char* message) {
    compute_delta();

    char *call_str = NULL;
    char *loc = get_location(call);

    asprintf(&call_str, "\"%s\"", get_call(call));

    p_print("error", NULL, call_str);

    if (loc) free(loc);
    if (call_str) free(call_str);

    last = timestamp();
}

static void trace_promises_vector_alloc(int sexptype, long length, long bytes, const char* srcref) {
    compute_delta();
    p_print("vector-alloc", NULL, NULL);
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

static void trace_promises_gc_entry(R_size_t size_needed) {
    compute_delta();
    p_print("builtin-entry", NULL, "gc_internal");
    last = timestamp();
}

static void trace_promises_gc_exit(int gc_count, double vcells, double ncells) {
    compute_delta();
    p_print("builtin-exit", NULL, "gc_internal");
    last = timestamp();
}

// TODO what's this?! if method call, then pertinent, otherwise maybe not?
static void trace_promises_S3_generic_entry(const char *generic, const SEXP object) {
    compute_delta();

    p_print("s3-generic-entry", NULL, generic);

    last = timestamp();
}

static void trace_promises_S3_generic_exit(const char *generic, const SEXP object, const SEXP retval) {
    compute_delta();

    p_print("s3-generic-exit", NULL, generic);

    last = timestamp();
}

static void trace_promises_S3_dispatch_entry(const char *generic, const char *clazz, const SEXP method, const SEXP object) {
    compute_delta();

    p_print("s3-dispatch-entry", NULL, get_name(method));

    last = timestamp();
}

static void trace_promises_S3_dispatch_exit(const char *generic, const char *clazz, const SEXP method, const SEXP object, const SEXP retval) {
    compute_delta();

    p_print("s3-dispatch-exit", NULL, get_name(method));

    last = timestamp();
}

static const rdt_handler trace_promises_rdt_handler = {
        &trace_promises_begin,
        NULL, // ?
        &trace_promises_function_entry,
        &trace_promises_function_exit,
        &trace_promises_builtin_entry,
        &trace_promises_builtin_exit,
        &trace_promises_force_promise_entry,
        &trace_promises_force_promise_exit,
        &trace_promises_promise_lookup,
        &trace_promises_error,
        &trace_promises_vector_alloc,
        NULL, // &trace_eval_entry,
        NULL, // &trace_eval_exit,
        &trace_promises_gc_entry,
        &trace_promises_gc_exit,
        &trace_promises_S3_generic_entry,
        &trace_promises_S3_generic_exit,
        &trace_promises_S3_dispatch_entry,
        &trace_promises_S3_dispatch_exit
};

rdt_handler *setup_promise_tracing(SEXP options) {
    const char *filename = get_string(get_named_list_element(options, "filename"));
    output = filename != NULL ? fopen(filename, "wt") : stderr;

    if (!output) {
        error("Unable to open %s: %s\n", filename, strerror(errno));
        return NULL;
    }

    rdt_handler *h = (rdt_handler *)  malloc(sizeof(rdt_handler));
    memcpy(h, &trace_promises_rdt_handler, sizeof(rdt_handler));

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
