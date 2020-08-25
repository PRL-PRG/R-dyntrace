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

void dyntracer_set_data(dyntracer_t* dyntracer, void* data) {
    dyntracer->data = data;
}

int dyntracer_has_data(dyntracer_t* dyntracer) {
    dyntracer->data != NULL;
}

void* dyntracer_get_data(dyntracer_t* dyntracer) {
    return dyntracer->data;
}

int dyntracer_has_dyntrace_entry_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.dyntrace_entry != NULL;
}
int dyntracer_has_dyntrace_exit_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.dyntrace_exit != NULL;
}
int dyntracer_has_deserialize_object_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.deserialize_object != NULL;
}
int dyntracer_has_closure_argument_list_creation_entry_callback(
    dyntracer_t* dyntracer) {
    return dyntracer->callback.closure_argument_list_creation_entry !=
           NULL;
}
int dyntracer_has_closure_argument_list_creation_exit_callback(
    dyntracer_t* dyntracer) {
    return dyntracer->callback.closure_argument_list_creation_exit !=
           NULL;
}
int dyntracer_has_closure_entry_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.closure_entry != NULL;
}
int dyntracer_has_closure_exit_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.closure_exit != NULL;
}
int dyntracer_has_builtin_entry_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.builtin_entry != NULL;
}
int dyntracer_has_builtin_exit_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.builtin_exit != NULL;
}
int dyntracer_has_special_entry_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.special_entry != NULL;
}
int dyntracer_has_special_exit_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.special_exit != NULL;
}
int dyntracer_has_substitute_call_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.substitute_call != NULL;
}
int dyntracer_has_assignment_call_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.assignment_call != NULL;
}
int dyntracer_has_promise_force_entry_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.promise_force_entry != NULL;
}
int dyntracer_has_promise_force_exit_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.promise_force_exit != NULL;
}
int dyntracer_has_promise_value_lookup_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.promise_value_lookup != NULL;
}
int dyntracer_has_promise_value_assign_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.promise_value_assign != NULL;
}
int dyntracer_has_promise_expression_lookup_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.promise_expression_lookup != NULL;
}
int dyntracer_has_promise_expression_assign_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.promise_expression_assign != NULL;
}
int dyntracer_has_promise_environment_lookup_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.promise_environment_lookup != NULL;
}
int dyntracer_has_promise_environment_assign_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.promise_environment_assign != NULL;
}
int dyntracer_has_promise_substitute_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.promise_substitute != NULL;
}
int dyntracer_has_eval_entry_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.eval_entry != NULL;
}
int dyntracer_has_eval_exit_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.eval_exit != NULL;
}
int dyntracer_has_gc_entry_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.gc_entry != NULL;
}
int dyntracer_has_gc_exit_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.gc_exit != NULL;
}
int dyntracer_has_gc_unmark_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.gc_unmark != NULL;
}
int dyntracer_has_gc_allocate_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.gc_allocate != NULL;
}
int dyntracer_has_context_entry_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.context_entry != NULL;
}
int dyntracer_has_context_exit_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.context_exit != NULL;
}
int dyntracer_has_context_jump_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.context_jump != NULL;
}
int dyntracer_has_S3_generic_entry_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.S3_generic_entry != NULL;
}
int dyntracer_has_S3_generic_exit_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.S3_generic_exit != NULL;
}
int dyntracer_has_S3_dispatch_entry_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.S3_dispatch_entry != NULL;
}
int dyntracer_has_S3_dispatch_exit_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.S3_dispatch_exit != NULL;
}
int dyntracer_has_S4_generic_entry_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.S4_generic_entry != NULL;
}
int dyntracer_has_S4_generic_exit_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.S4_generic_exit != NULL;
}
int dyntracer_has_S4_dispatch_argument_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.S4_dispatch_argument != NULL;
}
int dyntracer_has_environment_variable_define_callback(
    dyntracer_t* dyntracer) {
    return dyntracer->callback.environment_variable_define != NULL;
}
int dyntracer_has_environment_variable_assign_callback(
    dyntracer_t* dyntracer) {
    return dyntracer->callback.environment_variable_assign != NULL;
}
int dyntracer_has_environment_variable_remove_callback(
    dyntracer_t* dyntracer) {
    return dyntracer->callback.environment_variable_remove != NULL;
}
int dyntracer_has_environment_variable_lookup_callback(
    dyntracer_t* dyntracer) {
    return dyntracer->callback.environment_variable_lookup != NULL;
}
int dyntracer_has_environment_variable_exists_callback(
    dyntracer_t* dyntracer) {
    return dyntracer->callback.environment_variable_exists != NULL;
}
int dyntracer_has_environment_context_sensitive_promise_eval_entry_callback(
    dyntracer_t* dyntracer) {
    return dyntracer->callback.environment_context_sensitive_promise_eval_entry !=
           NULL;
}
int dyntracer_has_environment_context_sensitive_promise_eval_exit_callback(
    dyntracer_t* dyntracer) {
    return dyntracer->callback.environment_context_sensitive_promise_eval_exit !=
           NULL;
}

dyntrace_entry_callback_t
dyntracer_get_dyntrace_entry_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.dyntrace_entry;
}
dyntrace_exit_callback_t
dyntracer_get_dyntrace_exit_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.dyntrace_exit;
}
deserialize_object_callback_t
dyntracer_get_deserialize_object_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.deserialize_object;
}
closure_argument_list_creation_entry_callback_t
dyntracer_get_closure_argument_list_creation_entry_callback(
    dyntracer_t* dyntracer) {
    return dyntracer->callback.closure_argument_list_creation_entry;
}
closure_argument_list_creation_exit_callback_t
dyntracer_get_closure_argument_list_creation_exit_callback(
    dyntracer_t* dyntracer) {
    return dyntracer->callback.closure_argument_list_creation_exit;
}
closure_entry_callback_t
dyntracer_get_closure_entry_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.closure_entry;
}
closure_exit_callback_t
dyntracer_get_closure_exit_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.closure_exit;
}
builtin_entry_callback_t
dyntracer_get_builtin_entry_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.builtin_entry;
}
builtin_exit_callback_t
dyntracer_get_builtin_exit_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.builtin_exit;
}
special_entry_callback_t
dyntracer_get_special_entry_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.special_entry;
}
special_exit_callback_t
dyntracer_get_special_exit_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.special_exit;
}
substitute_call_callback_t
dyntracer_get_substitute_call_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.substitute_call;
}
assignment_call_callback_t
dyntracer_get_assignment_call_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.assignment_call;
}
promise_force_entry_callback_t
dyntracer_get_promise_force_entry_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.promise_force_entry;
}
promise_force_exit_callback_t
dyntracer_get_promise_force_exit_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.promise_force_exit;
}
promise_value_lookup_callback_t
dyntracer_get_promise_value_lookup_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.promise_value_lookup;
}
promise_value_assign_callback_t
dyntracer_get_promise_value_assign_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.promise_value_assign;
}
promise_expression_lookup_callback_t
dyntracer_get_promise_expression_lookup_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.promise_expression_lookup;
}
promise_expression_assign_callback_t
dyntracer_get_promise_expression_assign_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.promise_expression_assign;
}
promise_environment_lookup_callback_t
dyntracer_get_promise_environment_lookup_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.promise_environment_lookup;
}
promise_environment_assign_callback_t
dyntracer_get_promise_environment_assign_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.promise_environment_assign;
}
promise_substitute_callback_t
dyntracer_get_promise_substitute_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.promise_substitute;
}
eval_entry_callback_t
dyntracer_get_eval_entry_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.eval_entry;
}
eval_exit_callback_t dyntracer_get_eval_exit_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.eval_exit;
}
gc_entry_callback_t dyntracer_get_gc_entry_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.gc_entry;
}
gc_exit_callback_t dyntracer_get_gc_exit_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.gc_exit;
}
gc_unmark_callback_t dyntracer_get_gc_unmark_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.gc_unmark;
}
gc_allocate_callback_t
dyntracer_get_gc_allocate_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.gc_allocate;
}
context_entry_callback_t
dyntracer_get_context_entry_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.context_entry;
}
context_exit_callback_t
dyntracer_get_context_exit_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.context_exit;
}
context_jump_callback_t
dyntracer_get_context_jump_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.context_jump;
}
S3_generic_entry_callback_t
dyntracer_get_S3_generic_entry_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.S3_generic_entry;
}
S3_generic_exit_callback_t
dyntracer_get_S3_generic_exit_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.S3_generic_exit;
}
S3_dispatch_entry_callback_t
dyntracer_get_S3_dispatch_entry_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.S3_dispatch_entry;
}
S3_dispatch_exit_callback_t
dyntracer_get_S3_dispatch_exit_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.S3_dispatch_exit;
}
S4_generic_entry_callback_t
dyntracer_get_S4_generic_entry_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.S4_generic_entry;
}
S4_generic_exit_callback_t
dyntracer_get_S4_generic_exit_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.S4_generic_exit;
}
S4_dispatch_argument_callback_t
dyntracer_get_S4_dispatch_argument_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.S4_dispatch_argument;
}
environment_variable_define_callback_t
dyntracer_get_environment_variable_define_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.environment_variable_define;
}
environment_variable_assign_callback_t
dyntracer_get_environment_variable_assign_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.environment_variable_assign;
}
environment_variable_remove_callback_t
dyntracer_get_environment_variable_remove_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.environment_variable_remove;
}
environment_variable_lookup_callback_t
dyntracer_get_environment_variable_lookup_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.environment_variable_lookup;
}
environment_variable_exists_callback_t
dyntracer_get_environment_variable_exists_callback(dyntracer_t* dyntracer) {
    return dyntracer->callback.environment_variable_exists;
}
environment_context_sensitive_promise_eval_entry_callback_t
dyntracer_get_environment_context_sensitive_promise_eval_entry_callback(
    dyntracer_t* dyntracer) {
    return dyntracer->callback
        .environment_context_sensitive_promise_eval_entry;
}
environment_context_sensitive_promise_eval_exit_callback_t
dyntracer_get_environment_context_sensitive_promise_eval_exit_callback(
    dyntracer_t* dyntracer) {
    return dyntracer->callback
        .environment_context_sensitive_promise_eval_exit;
}

void dyntracer_set_dyntrace_entry_callback_t(dyntracer_t* dyntracer,
                                    dyntrace_entry_callback_t callback) {
    dyntracer->callback.dyntrace_entry = callback;
}

void dyntracer_set_dyntrace_exit_callback(dyntracer_t* dyntracer,
                                          dyntrace_exit_callback_t callback) {
    dyntracer->callback.dyntrace_exit = callback;
}
void dyntracer_set_deserialize_object_callback(
    dyntracer_t* dyntracer,
    deserialize_object_callback_t callback) {
    dyntracer->callback.deserialize_object = callback;
}
void dyntracer_set_closure_argument_list_creation_entry_callback(
    dyntracer_t* dyntracer,
    closure_argument_list_creation_entry_callback_t callback) {
    dyntracer->callback.closure_argument_list_creation_entry =
        callback;
}
void dyntracer_set_closure_argument_list_creation_exit_callback(
    dyntracer_t* dyntracer,
    closure_argument_list_creation_exit_callback_t callback) {
    dyntracer->callback.closure_argument_list_creation_exit = callback;
}
void dyntracer_set_closure_entry_callback(dyntracer_t* dyntracer,
                                          closure_entry_callback_t callback) {
    dyntracer->callback.closure_entry = callback;
}
void dyntracer_set_closure_exit_callback(dyntracer_t* dyntracer,
                                         closure_exit_callback_t callback) {
    dyntracer->callback.closure_exit = callback;
}
void dyntracer_set_builtin_entry_callback(dyntracer_t* dyntracer,
                                          builtin_entry_callback_t callback) {
    dyntracer->callback.builtin_entry = callback;
}
void dyntracer_set_builtin_exit_callback(dyntracer_t* dyntracer,
                                         builtin_exit_callback_t callback) {
    dyntracer->callback.builtin_exit = callback;
}
void dyntracer_set_special_entry_callback(dyntracer_t* dyntracer,
                                          special_entry_callback_t callback) {
    dyntracer->callback.special_entry = callback;
}
void dyntracer_set_special_exit_callback(dyntracer_t* dyntracer,
                                         special_exit_callback_t callback) {
    dyntracer->callback.special_exit = callback;
}
void dyntracer_set_substitute_call_callback(
    dyntracer_t* dyntracer,
    substitute_call_callback_t callback) {
    dyntracer->callback.substitute_call = callback;
}
void dyntracer_set_assignment_call_callback(
    dyntracer_t* dyntracer,
    assignment_call_callback_t callback) {
    dyntracer->callback.assignment_call = callback;
}
void dyntracer_set_promise_force_entry_callback(
    dyntracer_t* dyntracer,
    promise_force_entry_callback_t callback) {
    dyntracer->callback.promise_force_entry = callback;
}
void dyntracer_set_promise_force_exit_callback(
    dyntracer_t* dyntracer,
    promise_force_exit_callback_t callback) {
    dyntracer->callback.promise_force_exit = callback;
}
void dyntracer_set_promise_value_lookup_callback(
    dyntracer_t* dyntracer,
    promise_value_lookup_callback_t callback) {
    dyntracer->callback.promise_value_lookup = callback;
}
void dyntracer_set_promise_value_assign_callback(
    dyntracer_t* dyntracer,
    promise_value_assign_callback_t callback) {
    dyntracer->callback.promise_value_assign = callback;
}
void dyntracer_set_promise_expression_lookup_callback(
    dyntracer_t* dyntracer,
    promise_expression_lookup_callback_t callback) {
    dyntracer->callback.promise_expression_lookup = callback;
}
void dyntracer_set_promise_expression_assign_callback(
    dyntracer_t* dyntracer,
    promise_expression_assign_callback_t callback) {
    dyntracer->callback.promise_expression_assign = callback;
}
void dyntracer_set_promise_environment_lookup_callback(
    dyntracer_t* dyntracer,
    promise_environment_lookup_callback_t callback) {
    dyntracer->callback.promise_environment_lookup = callback;
}
void dyntracer_set_promise_environment_assign_callback(
    dyntracer_t* dyntracer,
    promise_environment_assign_callback_t callback) {
    dyntracer->callback.promise_environment_assign = callback;
}
void dyntracer_set_promise_substitute_callback(
    dyntracer_t* dyntracer,
    promise_substitute_callback_t callback) {
    dyntracer->callback.promise_substitute = callback;
}
void dyntracer_set_eval_entry_callback(dyntracer_t* dyntracer,
                                       eval_entry_callback_t callback) {
    dyntracer->callback.eval_entry = callback;
}
void dyntracer_set_eval_exit_callback(dyntracer_t* dyntracer,
                                      eval_exit_callback_t callback) {
    dyntracer->callback.eval_exit = callback;
}
void dyntracer_set_gc_entry_callback(dyntracer_t* dyntracer,
                                     gc_entry_callback_t callback) {
    dyntracer->callback.gc_entry = callback;
}
void dyntracer_set_gc_exit_callback(dyntracer_t* dyntracer,
                                    gc_exit_callback_t callback) {
    dyntracer->callback.gc_exit = callback;
}
void dyntracer_set_gc_unmark_callback(dyntracer_t* dyntracer,
                                      gc_unmark_callback_t callback) {
    dyntracer->callback.gc_unmark = callback;
}
void dyntracer_set_gc_allocate_callback(dyntracer_t* dyntracer,
                                        gc_allocate_callback_t callback) {
    dyntracer->callback.gc_allocate = callback;
}
void dyntracer_set_context_entry_callback(dyntracer_t* dyntracer,
                                          context_entry_callback_t callback) {
    dyntracer->callback.context_entry = callback;
}
void dyntracer_set_context_exit_callback(dyntracer_t* dyntracer,
                                         context_exit_callback_t callback) {
    dyntracer->callback.context_exit = callback;
}
void dyntracer_set_context_jump_callback(dyntracer_t* dyntracer,
                                         context_jump_callback_t callback) {
    dyntracer->callback.context_jump = callback;
}
void dyntracer_set_S3_generic_entry_callback(
    dyntracer_t* dyntracer,
    S3_generic_entry_callback_t callback) {
    dyntracer->callback.S3_generic_entry = callback;
}
void dyntracer_set_S3_generic_exit_callback(
    dyntracer_t* dyntracer,
    S3_generic_exit_callback_t callback) {
    dyntracer->callback.S3_generic_exit = callback;
}
void dyntracer_set_S3_dispatch_entry_callback(
    dyntracer_t* dyntracer,
    S3_dispatch_entry_callback_t callback) {
    dyntracer->callback.S3_dispatch_entry = callback;
}
void dyntracer_set_S3_dispatch_exit_callback(
    dyntracer_t* dyntracer,
    S3_dispatch_exit_callback_t callback) {
    dyntracer->callback.S3_dispatch_exit = callback;
}
void dyntracer_set_S4_generic_entry_callback(
    dyntracer_t* dyntracer,
    S4_generic_entry_callback_t callback) {
    dyntracer->callback.S4_generic_entry = callback;
}
void dyntracer_set_S4_generic_exit_callback(
    dyntracer_t* dyntracer,
    S4_generic_exit_callback_t callback) {
    dyntracer->callback.S4_generic_exit = callback;
}
void dyntracer_set_S4_dispatch_argument_callback(
    dyntracer_t* dyntracer,
    S4_dispatch_argument_callback_t callback) {
    dyntracer->callback.S4_dispatch_argument = callback;
}
void dyntracer_set_environment_variable_define_callback(
    dyntracer_t* dyntracer,
    environment_variable_define_callback_t callback) {
    dyntracer->callback.environment_variable_define = callback;
}
void dyntracer_set_environment_variable_assign_callback(
    dyntracer_t* dyntracer,
    environment_variable_assign_callback_t callback) {
    dyntracer->callback.environment_variable_assign = callback;
}
void dyntracer_set_environment_variable_remove_callback(
    dyntracer_t* dyntracer,
    environment_variable_remove_callback_t callback) {
    dyntracer->callback.environment_variable_remove = callback;
}
void dyntracer_set_environment_variable_lookup_callback(
    dyntracer_t* dyntracer,
    environment_variable_lookup_callback_t callback) {
    dyntracer->callback.environment_variable_lookup = callback;
}
void dyntracer_set_environment_variable_exists_callback(
    dyntracer_t* dyntracer,
    environment_variable_exists_callback_t callback) {
    dyntracer->callback.environment_variable_exists = callback;
}
void dyntracer_set_environment_context_sensitive_promise_eval_entry_callback(
    dyntracer_t* dyntracer,
    environment_context_sensitive_promise_eval_entry_callback_t callback) {
    dyntracer->callback
        .environment_context_sensitive_promise_eval_entry = callback;
}
void dyntracer_set_environment_context_sensitive_promise_eval_exit_callback(
    dyntracer_t* dyntracer,
    environment_context_sensitive_promise_eval_exit_callback_t callback) {
    dyntracer->callback
        .environment_context_sensitive_promise_eval_exit = callback;
}
