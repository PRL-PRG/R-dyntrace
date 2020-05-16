#include <Rdyntrace.h>
#include <Rinternals.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <deparse.h>

dyntracer_t *dyntrace_active_dyntracer = NULL;
int dyntrace_check_reentrancy = 1;
const char *dyntrace_active_dyntracer_probe_name = NULL;
int dyntrace_garbage_collector_state = 0;
int dyntrace_privileged_mode_flag = 0;

int dyntrace_probe_dyntrace_entry_disabled = 0;
int dyntrace_probe_dyntrace_exit_disabled = 0;
int dyntrace_probe_deserialize_object_disabled = 0;
int dyntrace_probe_closure_argument_list_creation_entry_disabled = 0;
int dyntrace_probe_closure_argument_list_creation_exit_disabled = 0;
int dyntrace_probe_closure_entry_disabled = 0;
int dyntrace_probe_closure_exit_disabled = 0;
int dyntrace_probe_builtin_entry_disabled = 0;
int dyntrace_probe_builtin_exit_disabled = 0;
int dyntrace_probe_special_entry_disabled = 0;
int dyntrace_probe_special_exit_disabled = 0;
int dyntrace_probe_promise_force_entry_disabled = 0;
int dyntrace_probe_promise_force_exit_disabled = 0;
int dyntrace_probe_promise_value_lookup_disabled = 0;
int dyntrace_probe_promise_value_assign_disabled = 0;
int dyntrace_probe_promise_expression_lookup_disabled = 0;
int dyntrace_probe_promise_expression_assign_disabled = 0;
int dyntrace_probe_promise_environment_lookup_disabled = 0;
int dyntrace_probe_promise_environment_assign_disabled = 0;
int dyntrace_probe_substitute_call_disabled = 0;
int dyntrace_probe_assignment_call_disabled = 0;
int dyntrace_probe_promise_substitute_disabled = 0;
int dyntrace_probe_eval_entry_disabled = 0;
int dyntrace_probe_eval_exit_disabled = 0;
int dyntrace_probe_gc_entry_disabled = 0;
int dyntrace_probe_gc_exit_disabled = 0;
int dyntrace_probe_gc_unmark_disabled = 0;
int dyntrace_probe_gc_allocate_disabled = 0;
int dyntrace_probe_context_entry_disabled = 0;
int dyntrace_probe_context_exit_disabled = 0;
int dyntrace_probe_context_jump_disabled = 0;
int dyntrace_probe_S3_generic_entry_disabled = 0;
int dyntrace_probe_S3_generic_exit_disabled = 0;
int dyntrace_probe_S3_dispatch_entry_disabled = 0;
int dyntrace_probe_S3_dispatch_exit_disabled = 0;
int dyntrace_probe_S4_generic_entry_disabled = 0;
int dyntrace_probe_S4_generic_exit_disabled = 0;
int dyntrace_probe_S4_dispatch_entry_disabled = 0;
int dyntrace_probe_S4_dispatch_exit_disabled = 0;
int dyntrace_probe_S4_dispatch_argument_disabled = 0;
int dyntrace_probe_environment_variable_define_disabled = 0;
int dyntrace_probe_environment_variable_assign_disabled = 0;
int dyntrace_probe_environment_variable_remove_disabled = 0;
int dyntrace_probe_environment_variable_lookup_disabled = 0;
int dyntrace_probe_environment_variable_exists_disabled = 0;
int dyntrace_probe_environment_context_sensitive_promise_eval_entry_disabled = 0;
int dyntrace_probe_environment_context_sensitive_promise_eval_exit_disabled = 0;

dyntracer_t *dyntracer_from_sexp(SEXP dyntracer_sexp) {
    return (dyntracer_t *)R_ExternalPtrAddr(dyntracer_sexp);
}

SEXP dyntracer_to_sexp(dyntracer_t *dyntracer, const char *classname) {
    SEXP dyntracer_sexp;
    PROTECT(dyntracer_sexp =
                R_MakeExternalPtr(dyntracer, install(classname), R_NilValue));
    SEXP dyntracer_class = PROTECT(allocVector(STRSXP, 1));
    SET_STRING_ELT(dyntracer_class, 0, mkChar(classname));
    classgets(dyntracer_sexp, dyntracer_class);
    UNPROTECT(2);
    return dyntracer_sexp;
}

dyntracer_t *dyntracer_replace_sexp(SEXP dyntracer_sexp,
                                    dyntracer_t *new_dyntracer) {
    dyntracer_t *old_dyntracer = R_ExternalPtrAddr(dyntracer_sexp);
    R_SetExternalPtrAddr(dyntracer_sexp, new_dyntracer);
    return old_dyntracer;
}

SEXP dyntracer_destroy_sexp(SEXP dyntracer_sexp,
                            void (*destroy_dyntracer)(dyntracer_t *dyntracer)) {
    dyntracer_t *dyntracer = dyntracer_replace_sexp(dyntracer_sexp, NULL);
    /* free dyntracer iff it has not already been freed.
       this check ensures that multiple calls to destroy_dyntracer on the same
       object do not crash the process. */
    if (dyntracer != NULL)
        destroy_dyntracer(dyntracer);
    return R_NilValue;
}

void dyntrace_enable_privileged_mode() { dyntrace_privileged_mode_flag = 1; }

void dyntrace_disable_privileged_mode() { dyntrace_privileged_mode_flag = 0; }

int dyntrace_is_priviliged_mode() { return dyntrace_privileged_mode_flag; }


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
    (char *)malloc(DYNTRACE_PARSE_DATA_BUFFER_SIZE);
  dyntrace_parse_data.strvec =
    PROTECT(allocVector(STRSXP, DYNTRACE_PARSE_DATA_MAXLINES));
}

static void deallocate_dyntrace_parse_data() {
  free(dyntrace_parse_data.buffer.data);
  UNPROTECT(1);
}

SEXP do_dyntrace(SEXP call, SEXP op, SEXP args, SEXP rho) {
    int eval_error = FALSE;
    SEXP code_block, expression, environment, result;
    dyntracer_t *dyntracer = NULL;
    dyntracer_t *dyntrace_previous_dyntracer = dyntrace_active_dyntracer;

    dyntrace_active_dyntracer = NULL;

    /* set this to NULL to let allocation happen below
       without any probes getting executed. */
    allocate_dyntrace_parse_data();

    /* extract objects from argument list */
    dyntracer = dyntracer_from_sexp(eval(CAR(args), rho));
    code_block = findVar(CADR(args), rho);
    PROTECT(expression = PRCODE(code_block));
    PROTECT(environment = PRENV(code_block));

    if (dyntracer == NULL)
        error("dyntracer is NULL");

    dyntrace_active_dyntracer = dyntracer;

    DYNTRACE_PROBE_DYNTRACE_ENTRY(expression, environment);

    PROTECT(result = R_tryEval(expression, environment, &eval_error));

    /* we want to return a sensible result for
       evaluation of the expression */
    if (eval_error) {
        result = R_NilValue;
    }

    DYNTRACE_PROBE_DYNTRACE_EXIT(expression, environment, result, eval_error);

    /* set this to NULL to let deallocation happen below
       without any probes getting executed. */
    dyntrace_active_dyntracer = NULL;

    deallocate_dyntrace_parse_data();

    dyntrace_active_dyntracer = dyntrace_previous_dyntracer;

    UNPROTECT(3);
    return result;
}

//-----------------------------------------------------------------------------
// helpers
//-----------------------------------------------------------------------------

int dyntrace_is_active() {
    return (dyntrace_active_dyntracer_probe_name != NULL);
}

dyntracer_t *dyntrace_get_active_dyntracer() {
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
  dyntrace_enable_privileged_mode();
  dyntrace_parse_data.buffer.data[0] = '\0';
  dyntrace_parse_data.len = 0;
  dyntrace_parse_data.linenumber = 0;
  dyntrace_parse_data.indent = 0;
  dyntrace_parse_data.opts = opts;
  deparse2buff(s, &dyntrace_parse_data);
  writeline(&dyntrace_parse_data);
  *linecount = dyntrace_parse_data.linenumber;
  dyntrace_disable_privileged_mode();
  return dyntrace_parse_data.strvec;
}

int newhashpjw(const char *s) { return R_Newhashpjw(s); }

SEXP dyntrace_lookup_environment(SEXP rho, SEXP key) {
  dyntrace_disable_probe(probe_environment_variable_lookup);
  SEXP value = R_UnboundValue;
  if (DDVAL(key)) {
      value = ddfindVar(key, rho);
  }
  else {
    value = findVar(key, rho);
  }
  dyntrace_enable_probe(probe_environment_variable_lookup);
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

int dyntrace_get_primitive_offset(SEXP op) { return (op)->u.primsxp.offset; }

const char* const dyntrace_get_c_function_name(SEXP op) {
    int offset = dyntrace_get_primitive_offset(op);
    return R_FunTab[offset].name;
}

SEXP* dyntrace_get_symbol_table() {
    return R_SymbolTable;
}
