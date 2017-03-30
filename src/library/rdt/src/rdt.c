#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include <Defn.h>

#include "rdt.h"

static rdt_handler *handler = NULL;

static void internal_eval(void *data) {
    void **args = data;
    SEXP block = (SEXP) args[0];
    SEXP rho = (SEXP) args[1];

    eval(block, rho);
}

SEXP Rdt(SEXP tracer, SEXP rho, SEXP options) {
    if (rdt_is_running()) {
        if (handler) free(handler);

        rdt_stop();
        return R_TrueValue;
    }

    if (!isEnvironment(rho)) {
	    error("'rho' must be an environment not %s", type2char(TYPEOF(rho)));
        return R_FalseValue;
    }

    const char *sys = get_string(tracer);    
    if (!strcmp("default", sys)) {
        handler = setup_default_tracing(options);
    } else if (!strcmp("noop", sys)) {
        handler = setup_noop_tracing(options);
    } else if (!strcmp("promises", sys)) {
        handler = setup_promise_tracing(options);
    } else if (!strcmp("debug", sys)) {
        handler = setup_debug_tracing(options);
    } else if (!strcmp("specialsxp", sys)) {
        handler = setup_specialsxp_tracing(options);
    }

    if (!handler) {
        error("Unable to initialize dynamic tracing");
        return R_FalseValue;
    }

    SEXP block = findVar(install("block"), rho);
    if (block == NULL || block == R_NilValue) {
        error("Unable to find 'block' variable");
        return R_FalseValue;
    }

    rdt_start(handler, block);

    // this is to prevent long jumps return earlier than needed
    void *data[2] = {block, rho};
    R_ToplevelExec(&internal_eval, (void *)data);

    if (!strcmp("promises", sys)) {
        cleanup_promise_tracing(options);
    }

    rdt_stop();
    return R_TrueValue; // TODO Why does this return TRUE and not the return value of the expression anyway?
}

inline const char *get_value_from_list(SEXP options) {
    //if (sexp == R_NilValue || TYPEOF(sexp) != )
}

inline const char *get_string(SEXP sexp) {
    if (sexp == R_NilValue || TYPEOF(sexp) != STRSXP) {
        return NULL;
    }

    return CHAR(STRING_ELT(sexp, 0));
}