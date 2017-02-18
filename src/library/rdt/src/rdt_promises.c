#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
//#include <Defn.h>
#include "../../../main/inspect.h"

#include "rdt.h"

// If defined printout will include increasing indents showing function calls.
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
#endif

    fprintf(output,
#ifdef RDT_PROMISES_INDENT
            "%s%s loc(%s) %s\n",
            indent_string,
#else
            "%s%s loc(%s) %s\n",
#endif

            //delta,
            type,
            CHKSTR(loc),
            CHKSTR(name));

#ifdef RDT_PROMISES_INDENT
    if (indent_string)
        free(indent_string);
#endif
}

static inline void print_builtin(const char *type, const char *loc, const char *name, const char *id) {
#ifdef RDT_PROMISES_INDENT
    char *indent_string = mk_indent();
#endif

    fprintf(output,
#ifdef RDT_PROMISES_INDENT
            "%s%s loc(%s) function(%s=%s)\n",
            indent_string,
#else
            "%s loc(%s) function(%s=%s)\n",
#endif
            //delta,
            type,
            CHKSTR(loc),
            CHKSTR(name),
            CHKSTR(id));

#ifdef RDT_PROMISES_INDENT
    if (indent_string)
        free(indent_string);
#endif
}

static inline void print_promise(const char *type, const char *loc, const char *name, const char *id) {
#ifdef RDT_PROMISES_INDENT
    char *indent_string = mk_indent();
#endif

    fprintf(output,
#ifdef RDT_PROMISES_INDENT
            "%s%s loc(%s) promise(%s=%s)\n",
            indent_string,
#else
            "%s loc(%s) promise(%s=%s)\n",
#endif
            //delta,
            type,
            CHKSTR(loc),
            CHKSTR(name),
            CHKSTR(id));

#ifdef RDT_PROMISES_INDENT
    if (indent_string)
        free(indent_string);
#endif
}

static inline char *concat_arguments(const char **arguments, /*const char **default_values, const char **promises,*/ int arguments_length) {
    int characters = 0;
    for (int i = 0; i<arguments_length; i++) {
        characters += strlen(arguments[i]); /* for the argument name */
        //if (default_values[i] != NULL)
        //    characters += strlen(default_values[i]) /* for the expression */ + 1 /* for "=" */;
        //characters += strlen(promises[i]);
    }

    char *argument_string = calloc(1, sizeof(char) * (characters + 1 * (arguments_length - 1)/* commas */ + 1 /* terminator */));

    for (int i=0; i<arguments_length; i++) {
        if (i)
            argument_string = strcat(argument_string, ",");

        argument_string = strcat(argument_string, arguments[i]);
//        argument_string = strcat(argument_string, promises[i]);
//
//        if (default_values[i] != NULL) {
//            argument_string = strcat(argument_string, "=");
//            argument_string = strcat(argument_string, default_values[i]);
//        }
    }

    return argument_string;
}

static inline char *concat_promises(const char **arguments, /*const char **default_values,*/ const char **promises, int arguments_length) {
    int characters = 0;
    for (int i = 0; i<arguments_length; i++) {
        characters += strlen(arguments[i]); /* for the argument name */
        //if (default_values[i] != NULL)
        //    characters += strlen(default_values[i]) /* for the expression */ + 1 /* for "=" */;
        characters += strlen(promises[i]) + 1; /* 1 is for "=" */
    }

    char *promises_string = calloc(1, sizeof(char) * (characters + 1 * (arguments_length - 1)/* commas */ + 1 /* terminator */));

    for (int i=0; i<arguments_length; i++) {
        if (i)
            promises_string = strcat(promises_string, ",");

        promises_string = strcat(promises_string, arguments[i]);
        promises_string = strcat(promises_string, "=");
        promises_string = strcat(promises_string, promises[i]);

        //if (default_values[i] != NULL) {
        //  promises_string = strcat(promises_string, "=");
        //  promises_string = strcat(promises_string, default_values[i]);
        //}
    }

    return promises_string;
}

static inline void print_function(const char *type, const char *loc, const char *name, const char *function_id, const char **arguments, /*const char **default_values,*/ const char **promises, const int arguments_num) {
#ifdef RDT_PROMISES_INDENT
    char *indent_string = mk_indent();
#endif

    char *argument_string = concat_arguments(arguments, /* default_values, promises, */ arguments_num);
    char *promises_string = concat_promises(arguments, /* default_values,*/ promises, arguments_num);

    fprintf(output,
#ifdef RDT_PROMISES_INDENT
        "%s%s loc(%s) function(%s=%s) params(%s) promises(%s)\n",
        indent_string,
#else
        "%s loc(%s) function(%s=%s) params(%s) promises(%s)\n",
#endif
        type,
        CHKSTR(loc),
        CHKSTR(name),
        CHKSTR(function_id),
        argument_string,
        promises_string
    );

#ifdef RDT_PROMISES_INDENT
    if (indent_string)
        free(indent_string);
#endif
    if (argument_string)
        free(argument_string);
    if (promises_string)
        free(promises_string);
}

static inline void print_function_bare(const char *type, const char *loc, const char *name, const char *function_id, const char **arguments, int arguments_num) {
#ifdef RDT_PROMISES_INDENT
    char *indent_string = mk_indent();
#endif

    char *argument_string = concat_arguments(arguments, /* default_values, promises, */ arguments_num);
    //char *promises_string = concat_promises(arguments, /* default_values,*/ promises, arguments_num);

    fprintf(output,
#ifdef RDT_PROMISES_INDENT
            "%s%s loc(%s) function(%s=%s) params(%s)\n",
            indent_string,
#else
            "%s loc(%s) function(%s=%s) params(%s)\n",
#endif
            type,
            CHKSTR(loc),
            CHKSTR(name),
            CHKSTR(function_id),
            argument_string
            //promises_string
    );

#ifdef RDT_PROMISES_INDENT
    if (indent_string)
        free(indent_string);
#endif
    if (argument_string)
        free(argument_string);
    //if (promises_string)
    //    free(promises_string);
}

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

//static inline char *trim_string(char *str) {
//    if (str == NULL)
//        return str;
//
//    int offset_bow = 0, offset_aft = 0;
//    char *aft = str + strlen(str) - 1;
//
//    for (; isspace((unsigned char) *(str + offset_bow)); offset_bow++);
//    for (; isspace((unsigned char) *(aft + offset_aft)); offset_aft--);
//
//    char *ret = malloc(sizeof(char *) * offset_aft - offset_bow + 1);
//    int ri = 0;
//    for (int si = offset_bow; si < offset_aft; si++, ri++)
//        ret[ri] = str[si];
//    ret[ri] = '\0';
//
//    return ret;
//}

//static inline char **strings_of_STRSXP(SEXP str, Rboolean flatten) {
//    // Currently I just want to handle a specific case here, so I'll return NULL for everything else.
//    if (TYPEOF(str) != STRSXP)
//        return NULL;
//
//    int size = XLENGTH(str); //count_elements(str);
//
//    char **strings = malloc((sizeof(char *) * size));
//    for (int i = 0; i < size; i++) {
//        Rprintf(">-----------------------[%d]\n", i);
//
//        strings[i] = strdup(CHAR(STRING_ELT(str, i)));
//
//        Rprintf("<-----------------------[%d] %s\n", i, strings[i]);
//    }
//}

//static inline char *flatten(char *str) {
//    int size = strlen(str);
//    for (int i = 0; i < size; i++)
//        if (str[i] == '\n') {
//            if (i)
//                if (str[i-1] == '{') {
//                    str[i] = ' ';
//                    continue;
//                }
//            if (i < size - 1)
//                if (str[i+1] == '}') {
//                    str[i] = ' ';
//                    continue;
//                }
//            str[i] = ';';
//        }
//    return str;
//}
//
//static inline char *remove_redundant_spaces(char *str) {
//    char *ret = malloc(sizeof(char) * (strlen(str) + 1));
//    int ret_size = 0;
//    int prec_is_space = 0;
//    for (int i = 0; str[i] != '\0'; i++)
//        if (str[i] == ' ' || str[i] == '\t') {
//            if (prec_is_space)
//                continue;
//            prec_is_space = 1;
//            ret[ret_size++] = ' ';
//        } else {
//            prec_is_space = 0;
//            ret[ret_size++] = str[i];
//        }
//    ret[ret_size] = '\0';
//    return ret;
//}

// TODO proper SEXP hashmap

static inline char *make_promise_id(SEXP promise) {
    char *promise_id = NULL;
    asprintf(&promise_id, "<P%p>", promise);
    return promise_id;
}

static inline char *make_function_id(SEXP function) {
    char *function_id = NULL;
    asprintf(&function_id, "<F%p>", function);
    return function_id;
}

static inline int get_arguments(SEXP op, SEXP rho, char ***return_arguments, /*char ***return_default_values,*/ char ***return_promises) {
    SEXP formals = FORMALS(op);

    int argument_count = count_elements(formals);

    char **arguments = malloc((sizeof(char *) * argument_count));
    //char **default_values = malloc ((sizeof(char *) * argument_count));
    char **promises = malloc ((sizeof(char *) * argument_count));

    for (int i=0; i<argument_count; i++, formals = CDR(formals)) {
        // Retrieve the argument name.
        SEXP argument_expression = TAG(formals);
        arguments[i] = strdup(get_name(argument_expression));

        // FIXME dot-dot-dot

        // Retrieve the default expression for the argument.
        // SEXP default_value_expression = CAR(formals);
        // if (default_value_expression != R_MissingArg) {
        //     SEXP deparsed_expression = deparse1line(default_value_expression, FALSE);
        //     // deparsed_expression has everything we need, but is formatted for display on console, so we de-prettify
        //     // it.
        //     char *flat_code = flatten(strdup(CHAR(STRING_ELT(deparsed_expression, 0))));
        //     default_values[i] = remove_redundant_spaces(flat_code);
        //     free(flat_code);
        // } else
        //     default_values[i] = NULL;

        // Retrieve the promise for the argument.
        // The call SEXP only contains AST to find the actual argument value, we need to search the environment.
        SEXP promise_expression = findVar(argument_expression, rho);
        //asprintf(&promises[i], "[%p]", promise_expression);
        promises[i] = make_promise_id(promise_expression);
        //Rprintf("promise=%s\n",promises[i]);
    }

    (*return_arguments) = arguments;
    //(*return_default_values) = default_values;
    (*return_promises) = promises;
    return argument_count;
}

// Triggggerrredd when entering function evaluation.
// TODO: function name, unique function identifier, arguments and their order, promises
// ??? where are promises created? and do we care?
// ??? will address of funciton change? garbage collector?
static void trace_promises_function_entry(const SEXP call, const SEXP op, const SEXP rho) {
    compute_delta();

    const char *type = is_byte_compiled(call) ? "=> bcod" : "=> func";
    const char *name = get_name(call);
    const char *ns = get_ns_name(op);
    const char *id = make_function_id(op);
    char *loc = get_location(op);
    char *fqfn = NULL;

    if (ns) {
        asprintf(&fqfn, "%s::%s", ns, CHKSTR(name));
    } else {
        fqfn = name != NULL ? strdup(name) : NULL;
    }

    char **arguments;
    //char **default_values;
    char **promises;
    int argument_count;

    argument_count = get_arguments(op, rho, &arguments, /*&default_values,*/ &promises);
    print_function(type, loc, name, id, (const char **)arguments, /*default_values,*/ (const char **)promises, argument_count);

    #ifdef RDT_PROMISES_INDENT
    indent++;
    #endif


    if (loc)
        free(loc);
    if (fqfn)
        free(fqfn);

    if (arguments)
        free(arguments);
    //Rprintf("<o.o<\n");

    //if (default_values)
    //    free(default_values);
    //Rprintf("^o.o^\n");

    if (promises)
        free(promises);
    //Rprintf(">o.o>\n");

    last = timestamp();
}

static void trace_promises_function_exit(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
    compute_delta();

    #ifdef RDT_PROMISES_INDENT
    indent--;
    #endif

    const char *type = is_byte_compiled(call) ? "<= bcod" : "<= func";
    const char *name = get_name(call);
    const char *ns = get_ns_name(op);
    const char *id = make_function_id(op);
    char *loc = get_location(op);
    char *fqfn = NULL;

    if (ns) {
        asprintf(&fqfn, "%s::%s", ns, CHKSTR(name));
    } else {
        fqfn = name != NULL ? strdup(name) : NULL;
    }

    char **arguments;
    //char **default_values;
    char **promises;
    int argument_count;

    argument_count = get_arguments(op, rho, &arguments, /*&default_values,*/ &promises);
    print_function(type, loc, name, id, (const char **)arguments, /*default_values,*/ (const char **)promises, argument_count);

    if (loc)
        free(loc);
    if (fqfn)
        free(fqfn);


    if (arguments)
        free(arguments);
    //Rprintf("<o.o<\n");

    //if (default_values)
    //    free(default_values);
    //Rprintf("^o.o^\n");

    if (promises)
        free(promises);
    //Rprintf(">o.o>\n");

    last = timestamp();
}

// XXX Probably don't need this?
static void trace_promises_builtin_entry(const SEXP call, const SEXP op, const SEXP rho) {
    compute_delta();

    const char *name = get_name(call);
    const char *id = make_function_id(op);

    print_builtin("=> b-in", NULL, name, id);

    //R_inspect(call);

    last = timestamp();
}

static void trace_promises_builtin_exit(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
    compute_delta();

    const char *name = get_name(call);
    const char *id = make_function_id(op);

    print_builtin("<= b-in", NULL, name, id);

    last = timestamp();
}

// Promise is being used inside a function body for the first time.
// TODO name of promise, expression inside promise, value evaluated if available, (in the long term) connected to a function
// TODO get more info to hook from eval (eval.c::4401 and at least 1 more line)
static void trace_promises_force_promise_entry(const SEXP symbol, const SEXP rho) {
    compute_delta();

    const char *name = get_name(symbol);
    const char *id;

    if (TYPEOF(symbol) == PROMSXP) {
        //R_inspect(symbol);
        //asprintf(&symbol_id, "[%p]", symbol);
        id = make_promise_id(symbol);
    } else if (TYPEOF(symbol) == SYMSXP) {
        SEXP promise_expression = findVar(symbol, rho);
        //R_inspect(promise_expression);
        //asprintf(&promise_id, "[%p]", promise_expression);
        id = make_promise_id(symbol);
    }

    print_promise("=> prom", NULL, name, id);

    last = timestamp();
}

static void trace_promises_force_promise_exit(const SEXP symbol, const SEXP rho, const SEXP val) {
    compute_delta();

    const char *name = get_name(symbol);
    const char *id = NULL;

    print_promise("<= prom", NULL, name, id);

    last = timestamp();
}

static void trace_promises_promise_lookup(const SEXP symbol, const SEXP rho, const SEXP val) {
    compute_delta();

    const char *name = get_name(symbol);
    const char *id = NULL;

    print_promise("<> lkup", NULL, name, id);

    last = timestamp();
}

static void trace_promises_error(const SEXP call, const char* message) {
    compute_delta();

    char *call_str = NULL;
    char *loc = get_location(call);

    asprintf(&call_str, "\"%s\"", get_call(call));

    //p_print("error", NULL, call_str);

    if (loc) free(loc);
    if (call_str) free(call_str);

    last = timestamp();
}

static void trace_promises_vector_alloc(int sexptype, long length, long bytes, const char* srcref) {
    compute_delta();
    //p_print("vector-alloc", NULL, NULL);
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
    //p_print("builtin-entry", NULL, "gc_internal");
    last = timestamp();
}

static void trace_promises_gc_exit(int gc_count, double vcells, double ncells) {
    compute_delta();
    //p_print("builtin-exit", NULL, "gc_internal");
    last = timestamp();
}

static void trace_promises_S3_generic_entry(const char *generic, const SEXP object) {
    compute_delta();

    //p_print("s3-generic-entry", NULL, generic);

    last = timestamp();
}

static void trace_promises_S3_generic_exit(const char *generic, const SEXP object, const SEXP retval) {
    compute_delta();

    //p_print("s3-generic-exit", NULL, generic);

    last = timestamp();
}

static void trace_promises_S3_dispatch_entry(const char *generic, const char *clazz, const SEXP method, const SEXP object) {
    compute_delta();

    //p_print("s3-dispatch-entry", NULL, get_name(method));

    last = timestamp();
}

static void trace_promises_S3_dispatch_exit(const char *generic, const char *clazz, const SEXP method, const SEXP object, const SEXP retval) {
    compute_delta();

    //p_print("s3-dispatch-exit", NULL, get_name(method));

    last = timestamp();
}


// TODO properly turn off probes we don't use
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
