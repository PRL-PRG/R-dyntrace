#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include <Defn.h>

#include "rdt.h"
#include <dlfcn.h>

static rdt_handler *handler = NULL;

static void internal_eval(void *data) {
    SEXP * args = data;
    SEXP block = args[0];
    SEXP rho = args[1];
    eval(block, rho);
}

static void * get_symbol_address(const char * symbol, const char * library_filepath ) {
    void * handle = dlopen(library_filepath, RTLD_LAZY);
    if (handle) return dlsym(handle, symbol);
    else return NULL;
}

SEXP Rdt(SEXP tracer, SEXP library_filepath, SEXP rho, SEXP options) {
    if (rdt_is_running()) {
        if (handler) free(handler);

        rdt_stop();
        return R_TrueValue;
    }

    if (!isEnvironment(rho)) {
	    error("'rho' must be an environment not %s", type2char(TYPEOF(rho)));
        return R_FalseValue;
    }
    SEXP code_block = findVar(install("code_block"), rho);
    if (code_block == NULL || code_block == R_NilValue) {
        error("Unable to find 'block' variable");
        return R_FalseValue;
    }

    const char * location = get_string(library_filepath);
    tracer_setup_t setup_tracing = get_symbol_address("setup_tracing", location);
    tracer_cleanup_t cleanup_tracing = get_symbol_address("cleanup_tracing", location);

    if (!setup_tracing) {
        error("Tracer %s not found", get_string(tracer));
        return R_FalseValue;
    }

    handler = setup_tracing(options);

    if (!handler) {
      error("Unable to initialize dynamic tracing");
      return R_FalseValue;
    }

    rdt_start(handler, code_block);

    // this is to prevent long jumps return earlier than needed
    void *data[2] = {code_block, rho};
    R_ToplevelExec(&internal_eval, (void *)data);

    if (cleanup_tracing) {
        cleanup_tracing(options);
    }
    // Missing cleanup_tracing function is not an error

    rdt_stop();

    return R_TrueValue;
    // TODO Why does this return TRUE and not the return value of the expression anyway?
}

inline const char *get_string(SEXP sexp) {
  if (sexp == R_NilValue || TYPEOF(sexp) != STRSXP) {
    return NULL;
  }

  return CHAR(STRING_ELT(sexp, 0));
}
