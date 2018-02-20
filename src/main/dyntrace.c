#include <Rdyntrace.h>
#include <Rinternals.h>
#include <deparse.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if !defined(HAVE_CLOCK_GETTIME) && defined(__MACH__)
#include <mach/clock.h>
#include <mach/mach.h>
#endif

dyntrace_context_t *dyntrace_active_dyntrace_context = NULL;
dyntracer_t *dyntrace_active_dyntracer = NULL;
int dyntrace_check_reentrancy = 1;
const char *dyntrace_active_dyntracer_probe_name = NULL;
int dyntrace_garbage_collector_state = 0;
clock_t dyntrace_stopwatch;
int dyntrace_privileged_mode_flag = 0;

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

static const char *get_current_datetime() {
    time_t current_time = time(NULL);
    char *time_string = ctime(&current_time);
    return strtok(time_string, "\n"); // remove new-lines
}

static void
assign_environment_variables(environment_variables_t *environment_variables) {
    environment_variables->r_compile_pkgs = getenv("R_COMPILE_PKGS");
    environment_variables->r_disable_bytecode = getenv("R_DISABLE_BYTECODE");
    environment_variables->r_enable_jit = getenv("R_ENABLE_JIT");
    environment_variables->r_keep_pkg_source = getenv("R_KEEP_PKG_SOURCE");
}

static dyntracing_context_t *create_dyntracing_context() {
    // calloc ensures that execution times are all set to 0. If we malloc
    // instead, we have to explicitly set all execution times to 0;
    dyntracing_context_t *dyntracing_context =
        calloc(1, sizeof(dyntracing_context_t));
    assign_environment_variables(&(dyntracing_context->environment_variables));
    return dyntracing_context;
}

static dyntrace_context_t *create_dyntrace_context(dyntracer_t *dyntracer) {
    dyntrace_context_t *dyntrace_context =
        calloc(1, sizeof(dyntrace_context_t));
    dyntrace_context->dyntracer_context = dyntracer->context;
    dyntrace_context->dyntracing_context = create_dyntracing_context();
    return dyntrace_context;
}

static void destroy_dyntrace_context(dyntrace_context_t *dyntrace_context) {
    free(dyntrace_context->dyntracing_context);
    free(dyntrace_context);
}

SEXP do_dyntrace(SEXP call, SEXP op, SEXP args, SEXP rho) {
    int eval_error = FALSE;
    SEXP expression, environment, result;
    dyntracer_t *dyntracer = NULL;
    dyntracer_t *dyntrace_previous_dyntracer = dyntrace_active_dyntracer;

    /* extract objects from argument list */
    dyntracer = dyntracer_from_sexp(eval(CAR(args), rho));
    PROTECT(expression = findVar(CADR(args), rho));
    PROTECT(environment = eval(CADDR(args), rho));

    if (dyntracer == NULL)
        error("dyntracer is NULL");
    /* create dyntrace context */
    dyntrace_active_dyntrace_context = create_dyntrace_context(dyntracer);

    /* begin dyntracing */
    dyntrace_active_dyntracer = dyntracer;
    dyntrace_stopwatch = clock();
    dyntrace_active_dyntrace_context->dyntracing_context->begin_datetime =
        get_current_datetime();

    DYNTRACE_PROBE_BEGIN(expression);

    /* continue dyntracing */
    PROTECT(result = R_tryEval(expression, environment, &eval_error));
    if (eval_error) {
        /* we want to return a sensible result for
           evaluation of the expression */
        result = R_NilValue;
    } else {
      /* end dyntracing */
      dyntrace_active_dyntrace_context->dyntracing_context->end_datetime =
        get_current_datetime();
      DYNTRACE_PROBE_END();
    }

    dyntrace_active_dyntracer = dyntrace_previous_dyntracer;

    /* destroy dyntrace context */
    destroy_dyntrace_context(dyntrace_active_dyntrace_context);
    dyntrace_active_dyntrace_context = NULL;

    UNPROTECT(3);
    return result;
}

//-----------------------------------------------------------------------------
// helpers
//-----------------------------------------------------------------------------

int dyntrace_is_active() {
    return (dyntrace_active_dyntracer_probe_name != NULL);
}

void dyntrace_disable_garbage_collector() {
    dyntrace_garbage_collector_state = R_GCEnabled;
    R_GCEnabled = 0;
}

void dyntrace_reinstate_garbage_collector() {
    R_GCEnabled = dyntrace_garbage_collector_state;
}

clock_t dyntrace_reset_stopwatch() {
    clock_t end_stopwatch = clock();
    clock_t difference = end_stopwatch - dyntrace_stopwatch;
    dyntrace_stopwatch = end_stopwatch;
    return difference;
}

SEXP get_named_list_element(const SEXP list, const char *name) {
    if (TYPEOF(list) != VECSXP) {
        error("Not a list");
    }

    SEXP e = R_NilValue;
    SEXP names = getAttrib(list, R_NamesSymbol);

    for (int i = 0; i < length(list); i++) {
        if (strcmp(CHAR(STRING_ELT(names, i)), name) == 0) {
            e = VECTOR_ELT(list, i);
            break;
        }
    }

    return e;
}

char *serialize_sexp(SEXP s) {

    LocalParseData parse_data = {0,
                                 0,
                                 0,
                                 0,
                                 /*startline = */ TRUE,
                                 0,
                                 NULL,
                                 /*DeparseBuffer=*/{NULL, 0, 1024 * 1024}
                                 INT_MAX,
                                 FALSE,
                                 0,
                                 TRUE,
                                 FALSE,
                                 INT_MAX,
                                 TRUE,
                                 0,
                                 FALSE};
    parse_data.linenumber = 0;
    parse_data.indent = 0;
    deparse2buff(s, &parse_data);
    return parse_data.buffer.data;
}

inline const char *get_string(SEXP sexp) {
    if (sexp == R_NilValue || TYPEOF(sexp) != STRSXP) {
        return NULL;
    }

    return CHAR(STRING_ELT(sexp, 0));
}

int newhashpjw(const char *s) { return R_Newhashpjw(s); }
