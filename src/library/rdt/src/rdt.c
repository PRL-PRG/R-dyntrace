#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
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
    }

    if (!handler) {
        error("Unable to initialize dynamic tracing");
        return R_FalseValue;
    }

    rdt_start(handler);

    SEXP block = findVar(install("block"), rho);
    if (block == NULL || block == R_NilValue) {
        error("Unable to find 'block' variable");
        return R_FalseValue;
    }

    // this is to prevent long jumps return earlier than needed
    void *data[2] = {block, rho};
    R_ToplevelExec(&internal_eval, (void *)data);
    rdt_stop();
    return R_TrueValue; // TODO Why does this return TRUE and not the return value of the expression anyway?
}

SEXP Rdt_deparse(SEXP exp) {
    Rprintf("> %i (%s)\n", TYPEOF(exp), type2char(TYPEOF(exp)));

    if (TYPEOF(exp) == LISTSXP) {
        while (exp != R_NilValue) {
            SEXP tag = exp->u.listsxp.tagval;
            Rdt_deparse(tag);

            // TODO check if tag == ... etc.
            SEXP car = exp->u.listsxp.carval;
            if (car != R_MissingArg) {
                Rprintf("=");
                Rdt_deparse(car);
            }

            exp = exp->u.listsxp.cdrval; // l := tl l
        }
        return R_TrueValue;
    }

    if (TYPEOF(exp) == CLOSXP) {
        Rprintf("formals: ");
        Rdt_deparse(exp->u.closxp.formals); // -> LISTSXP
        Rprintf("body: ");
        Rdt_deparse(exp->u.closxp.body);

        return R_TrueValue;
    }

    if (TYPEOF(exp) == LANGSXP) {

    }

    if (TYPEOF(exp) == SYMSXP) {
        return Rdt_deparse(exp->u.symsxp.pname);
    }

    if (TYPEOF(exp) == CHARSXP) {
        const char *name = CHAR(exp);
        Rprintf(name);
        return R_TrueValue;
    }

    if (TYPEOF(exp) == REALSXP) {
        // TODO
    }

    return R_FalseValue;
}

inline const char *get_string(SEXP sexp) {
    if (sexp == R_NilValue || TYPEOF(sexp) != STRSXP) {
        return NULL;
    }

    return CHAR(STRING_ELT(sexp, 0));
}