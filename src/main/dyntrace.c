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

struct dyntrace_result_t dyntrace_trace_code(dyntracer_t* dyntracer, SEXP code, SEXP environment) {
    struct dyntrace_result_t result;
    result.error_code = 0;
    result.value = R_NilValue;
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

    PROTECT(result.value = R_tryEval(code, environment, &result.error_code));

    DYNTRACE_PROBE_DYNTRACE_EXIT(code, environment, result.value, result.error_code);

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
    dyntracer->data = NULL;
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

