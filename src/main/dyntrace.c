#include <Rdyntrace.h>
#include <Rinternals.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <deparse.h>

#if !defined(HAVE_CLOCK_GETTIME) && defined(__MACH__)
#include <mach/clock.h>
#include <mach/mach.h>
#endif

dyntrace_context_t * dyntrace_active_dyntrace_context = NULL;
dyntracer_t *dyntrace_active_dyntracer = NULL;
const char *dyntrace_active_dyntracer_probe_name = NULL;
int dyntrace_garbage_collector_state = 0;

dyntracer_t * dyntrace_unwrap_dyntracer(SEXP wrapped_dyntracer) {
    return (dyntracer_t *)R_ExternalPtrAddr(wrapped_dyntracer);
}

SEXP dyntrace_wrap_dyntracer(dyntracer_t *dyntracer,
                             const char *classname) {
    SEXP wrapped_dyntracer;
    PROTECT(wrapped_dyntracer = R_MakeExternalPtr(dyntracer,
                                                  install(classname),
                                                  R_NilValue));
    SEXP dyntracer_class = PROTECT(allocVector(STRSXP, 1));
    SET_STRING_ELT(dyntracer_class, 0, mkChar(classname));
    classgets(wrapped_dyntracer, dyntracer_class);
    UNPROTECT(2);
    return wrapped_dyntracer;
}

static dyntrace_context_t * create_dyntrace_context(dyntracer_t * dyntracer) {
  dyntrace_context_t * dyntrace_context = calloc(1, sizeof(dyntrace_context_t));
  dyntrace_context -> dyntracing_context =  calloc(1, sizeof(dyntracing_context_t));
  dyntrace_context -> dyntracer_context = dyntracer -> context;
  return dyntrace_context;
}

static void destroy_dyntrace_context(dyntrace_context_t * dyntrace_context) {
    free(dyntrace_context -> dyntracing_context);
    free(dyntrace_context);
}

SEXP do_dyntrace(SEXP call, SEXP op, SEXP args, SEXP rho) {
    int eval_error = FALSE;
    SEXP expression, environment, result;
    dyntracer_t * dyntracer = NULL;
    dyntracer_t * dyntrace_previous_dyntracer = dyntrace_active_dyntracer;

    /* extract objects from argument list */
    dyntracer = dyntrace_unwrap_dyntracer(eval(CAR(args), rho));
    PROTECT(expression = findVar(CADR(args), rho));
    PROTECT(environment = eval(CADDR(args), rho));
    
    /* create dyntrace context */
    dyntrace_active_dyntrace_context = create_dyntrace_context(dyntracer);    
    
    /* begin dyntracing */
    dyntrace_active_dyntracer = dyntracer;
    DYNTRACE_PROBE_BEGIN(expression);

    /* continue dyntracing */
    PROTECT(result = R_tryEval(expression, environment, &eval_error));
    if(eval_error) {
        REprintf("ERROR while dyntracing code block");
        result = R_NilValue;
    }

    /* end dyntracing */
    DYNTRACE_PROBE_END();
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
    return (dyntrace_active_dyntracer != NULL);
}

void dyntrace_disable_garbage_collector() {
    dyntrace_garbage_collector_state = R_GCEnabled;
    R_GCEnabled = 0;
}

void dyntrace_reinstate_garbage_collector() {
    R_GCEnabled = dyntrace_garbage_collector_state;
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

const char* serialize_sexp(SEXP s) {

  LocalParseData parse_data = {0, 0, 0, 0, /*startline = */TRUE, 0,
                               NULL, /*DeparseBuffer=*/{NULL, 0, BUFSIZE},
                               INT_MAX, FALSE, 0, TRUE, FALSE,
                               INT_MAX, TRUE, 0, FALSE};
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
