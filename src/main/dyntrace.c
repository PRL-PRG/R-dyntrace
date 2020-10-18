#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_CONFIG_H
#    include <config.h>
#endif
#include <Defn.h>
#include <deparse.h>
#include <Rdyntrace.h>

dyntracer_t* dyntrace_active_dyntracer = NULL;
const char* dyntrace_active_dyntracer_callback_name = NULL;
int dyntrace_garbage_collector_state = 0;
int dyntrace_status = 0;

//void dyntrace_status_initialize(int size) {
//    dyntrace_tracing_status.index = 0;
//    dyntrace_tracing_status.size = size;
//    dyntrace_tracing_status.status_stack = malloc(size * sizeof(int));
//}
//
//void dyntrace_status_finalize() {
//    dyntrace_tracing_status.index = 0;
//    dyntrace_tracing_status.size = 0;
//    free(dyntrace_tracing_status.status_stack);
//    dyntrace_tracing_status.status_stack = NULL;
//}
//
//void dyntrace_status_push(dyntrace_tracing_status_t* dyntrace_tracing_status,
//                          int status) {
//    int index = dyntrace_tracing_status->index;
//    int size = dyntrace_tracing_status->size;
//
//    if (dyntrace_tracing_status->status_stack == NULL) {
//        return;
//    }
//
//    if (index == size) {
//        dyntrace_tracing_status->status_stack =
//            realloc(dyntrace_tracing_status->status_stack, 2 * size);
//        dyntrace_tracing_status->size *= 2;
//    }
//
//    dyntrace_tracing_status->status_stack[index] = status;
//    dyntrace_tracing_status->index += 1;
//}
//
//void dyntrace_status_pop(dyntrace_tracing_status_t* dyntrace_tracing_status) {
//    int index = dyntrace_tracing_status->index;
//
//    if (index == 0) {
//        return;
//    }
//
//    dyntrace_tracing_status->index -= 1;
//}
//
//int dyntrace_status_peek(dyntrace_tracing_status_t* dyntrace_tracing_status) {
//    int index = dyntrace_tracing_status->index;
//    return dyntrace_tracing_status->status_stack[index - 1];
//}
//
//void dyntrace_enable_tracing() {
//    dyntrace_status_push(&dyntrace_tracing_status, 1);
//}
//
//void dyntrace_disable_tracing() {
//    dyntrace_status_push(&dyntrace_tracing_status, 0);
//}
//
//void dyntrace_reinstate_tracing() {
//    dyntrace_status_pop(&dyntrace_tracing_status);
//}
//
//int dyntrace_is_tracing_enabled() {
//    return dyntrace_status_peek(&dyntrace_tracing_status);
//}
//
//int dyntrace_is_tracing_disabled() {
//    return !dyntrace_is_tracing_enabled();
//}

#define DYNTRACE_PARSE_DATA_BUFFER_SIZE (1024 * 1024 * 8)
#define DYNTRACE_PARSE_DATA_MAXLINES (10000)

static LocalParseData dyntrace_parse_data = {
    0,
    0,
    0,
    0,
    /*startline = */ TRUE,
    0,
    NULL,
    /*DeparseBuffer=*/{NULL, 0, DYNTRACE_PARSE_DATA_BUFFER_SIZE},
    INT_MAX,
    FALSE,
    0,
    TRUE,
#ifdef longstring_WARN
    FALSE,
#endif
    DYNTRACE_PARSE_DATA_MAXLINES,
    TRUE,
    0,
    FALSE};

static void allocate_dyntrace_parse_data() {
    dyntrace_parse_data.buffer.data =
        (char*) malloc(DYNTRACE_PARSE_DATA_BUFFER_SIZE);
    dyntrace_parse_data.strvec =
        PROTECT(allocVector(STRSXP, DYNTRACE_PARSE_DATA_MAXLINES));
}

static void deallocate_dyntrace_parse_data() {
    free(dyntrace_parse_data.buffer.data);
    UNPROTECT(1);
}

//SEXP do_dyntrace(SEXP call, SEXP op, SEXP args, SEXP rho) {
//    SEXP r_dyntracer;
//    SEXP code;
//    SEXP environment;
//    dyntracer_t* dyntracer = NULL;
//
//    if (length(args) != 3) {
//        Rf_error("dyntrace expects three arguments.");
//    }
//
//    r_dyntracer = eval(CAR(args), rho);
//
//    if (TYPEOF(r_dyntracer) != EXTPTRSXP) {
//        Rf_error("first argument of dyntrace should be a dyntracer object.");
//    }
//
//    dyntracer = dyntracer_from_sexp(r_dyntracer);
//
//    if (dyntracer == NULL) {
//        Rf_error("dyntracer is NULL");
//    }
//
//    code = CADR(args);
//
//    environment = eval(CADDR(args), rho);
//
//    if (TYPEOF(environment) != ENVSXP) {
//        Rf_error("third argument of dyntrace should be an environment.");
//    }
//
//    return dyntrace_trace_code(dyntracer, code, environment);
//}

SEXP dyntrace_trace_code(dyntracer_t* dyntracer, SEXP code, SEXP environment) {
    int eval_error = FALSE;
    SEXP result;
    dyntracer_t* dyntrace_previous_dyntracer = dyntrace_active_dyntracer;

    PROTECT(code);
    PROTECT(environment);

    // dyntrace_status_initialize(100);

    dyntrace_active_dyntracer = NULL;

    /* set this to NULL to let allocation happen below
       without any callbacks getting executed. */
    allocate_dyntrace_parse_data();

    dyntrace_active_dyntracer = dyntracer;

    // dyntrace_enable_tracing();

    DYNTRACE_PROBE_DYNTRACE_ENTRY(code, environment);

    PROTECT(result = R_tryEval(code, environment, &eval_error));

    /* we want to return a sensible result for
       evaluation of the code */
    if (eval_error) {
        result = R_NilValue;
    }

    DYNTRACE_PROBE_DYNTRACE_EXIT(code, environment, result, eval_error);

    //dyntrace_reinstate_tracing();

    /* set this to NULL to let deallocation happen below
       without any callbacks getting executed. */
    dyntrace_active_dyntracer = NULL;

    deallocate_dyntrace_parse_data();

    dyntrace_active_dyntracer = dyntrace_previous_dyntracer;

    // dyntrace_status_finalize();

    UNPROTECT(3);

    return result;
}

//-----------------------------------------------------------------------------
// helpers
//-----------------------------------------------------------------------------

void dyntrace_enable() {
    dyntrace_status = 1;
}

void dyntrace_disable() {
    dyntrace_status = 0;
}

int dyntrace_is_enabled() {
    return dyntrace_status;
}

int dyntrace_is_active() {
    return (dyntrace_active_dyntracer_callback_name != NULL);
}

dyntracer_t* dyntrace_get_active_dyntracer() {
    return dyntrace_active_dyntracer;
}

void dyntrace_disable_garbage_collector() {
    dyntrace_garbage_collector_state = R_GCEnabled;
    R_GCEnabled = 0;
}

void dyntrace_reinstate_garbage_collector() {
    R_GCEnabled = dyntrace_garbage_collector_state;
}

SEXP serialize_sexp(SEXP s, int* linecount) {
    int opts = DELAYPROMISES | USESOURCE;
    //dyntrace_disable_tracing();
    dyntrace_parse_data.buffer.data[0] = '\0';
    dyntrace_parse_data.len = 0;
    dyntrace_parse_data.linenumber = 0;
    dyntrace_parse_data.indent = 0;
    dyntrace_parse_data.opts = opts;
    deparse2buff(s, &dyntrace_parse_data);
    writeline(&dyntrace_parse_data);
    *linecount = dyntrace_parse_data.linenumber;
    //dyntrace_enable_tracing();
    return dyntrace_parse_data.strvec;
}

int newhashpjw(const char* s) {
    return R_Newhashpjw(s);
}

SEXP dyntrace_lookup_environment(SEXP rho, SEXP key) {
    SEXP value = R_UnboundValue;
    if (DDVAL(key)) {
        value = ddfindVar(key, rho);
    } else {
        value = findVar(key, rho);
    }
    return value;
}

SEXP dyntrace_get_promise_expression(SEXP promise) {
    return (promise)->u.promsxp.expr;
}

SEXP dyntrace_get_promise_environment(SEXP promise) {
    return (promise)->u.promsxp.env;
}

SEXP dyntrace_get_promise_value(SEXP promise) {
    return (promise)->u.promsxp.value;
}

int dyntrace_get_c_function_argument_evaluation(SEXP op) {
    int offset = dyntrace_get_primitive_offset(op);
    return (R_FunTab[offset].eval) % 10;
}

int dyntrace_get_c_function_arity(SEXP op) {
    return PRIMARITY(op);
}

int dyntrace_get_primitive_offset(SEXP op) {
    return (op)->u.primsxp.offset;
}

const char* const dyntrace_get_c_function_name(SEXP op) {
    int offset = dyntrace_get_primitive_offset(op);
    return R_FunTab[offset].name;
}

SEXP* dyntrace_get_symbol_table() {
    return R_SymbolTable;
}

dyntracer_t* dyntracer_create(void* data) {
   /* calloc initializes the memory to zero. This ensures that probes not
      attached will be NULL. Replacing calloc with malloc will cause segfaults. */
    dyntracer_t* dyntracer = (dyntracer_t*) (calloc(1, sizeof(dyntracer_t)));
    dyntracer_set_data(dyntracer, data);
    return dyntracer;
}

void* dyntracer_destroy(dyntracer_t* dyntracer) {
    void* data = dyntracer_get_data(dyntracer);
    free(dyntracer);
    return data;
}

int dyntracer_has_data(dyntracer_t* dyntracer) {
    dyntracer->data != NULL;
}

void dyntracer_set_data(dyntracer_t* dyntracer, void* data) {
    dyntracer->data = data;
}

void* dyntracer_get_data(dyntracer_t* dyntracer) {
    return dyntracer->data;
}

void dyntracer_remove_data(dyntracer_t* dyntracer) {
    return dyntracer->data = NULL;
}

#define DYNTRACE_CALLBACK_API(NAME, ...)                                                        \
    int dyntracer_has_##NAME##_callback(dyntracer_t* dyntracer) {                               \
        return dyntracer->callback.NAME != NULL;                                                \
    }                                                                                           \
                                                                                                \
    NAME##_callback_t dyntracer_get_##NAME##_callback(dyntracer_t* dyntracer) {                 \
        return dyntracer->callback.NAME;                                                        \
    }                                                                                           \
                                                                                                \
    void dyntracer_set_##NAME##_callback(dyntracer_t* dyntracer, NAME##_callback_t callback) {  \
        dyntracer->callback.NAME = callback;                                                    \
    }                                                                                           \
                                                                                                \
    void dyntracer_remove_##NAME##_callback(dyntracer_t* dyntracer) {                           \
        dyntracer->callback.NAME = NULL;                                                        \
    }

DYNTRACE_CALLBACK_MAP(DYNTRACE_CALLBACK_API)

#undef DYNTRACE_CALLBACK_API

