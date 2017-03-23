#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rdtrace.h>
#include <time.h>

static const rdt_handler rdt_null_handler;

const rdt_handler *rdt_curr_handler = &rdt_null_handler;

//-----------------------------------------------------------------------------
// helpers
//-----------------------------------------------------------------------------

const char *get_ns_name(SEXP op) {
    SEXP env = CLOENV(op);
    SEXP spec = R_NamespaceEnvSpec(env);

    if (spec != R_NilValue) {
        if (TYPEOF(spec) == STRSXP && LENGTH(spec) > 0) {
            return CHAR(STRING_ELT(spec, 0));  
        } else if (TYPEOF(spec) == CHARSXP) {
            return CHAR(spec);
        } 
    }

    return NULL;
}

const char *get_name(SEXP sexp) {
    const char *s = NULL;

    switch(TYPEOF(sexp)) {
        case CHARSXP:
            s = CHAR(sexp);
            break;
        case LANGSXP:
            s = get_name(CAR(sexp));
            break;
        case BUILTINSXP:
            s = CHAR(PRIMNAME(sexp));
            break;
        case SYMSXP:
            s = CHAR(PRINTNAME(sexp));
            break;
    }

    return s;
}

static int get_lineno(SEXP srcref) {
    if (srcref && srcref != R_NilValue) {
        if (TYPEOF(srcref) == VECSXP) {
            srcref = VECTOR_ELT(srcref, 0);
        }

        return asInteger(srcref);
    } 
    
    return -1;               
}

static const char *get_filename(SEXP srcref) {
    if (srcref && srcref != R_NilValue) {
        if (TYPEOF(srcref) == VECSXP) srcref = VECTOR_ELT(srcref, 0);
        SEXP srcfile = getAttrib(srcref, R_SrcfileSymbol);
        if (TYPEOF(srcfile) == ENVSXP) {
            SEXP filename = findVar(install("filename"), srcfile);
            if (isString(filename) && length(filename)) {
                return CHAR(STRING_ELT(filename, 0));
            }
        }
    }
    
    return NULL;
}

char *get_location(SEXP op) {
    SEXP srcref = getAttrib(op, R_SrcrefSymbol);
    const char *filename = get_filename(srcref);
    int lineno = get_lineno(srcref);
    char *location = NULL;

    if (filename) {
        if (strlen(filename) > 0) {
            asprintf(&location, "%s:%d", filename, lineno);
        } else {
            location = strdup("<console>");
        }
    }

    return location;
}

const char *get_call(SEXP call) {
    return CHAR(STRING_ELT(deparse1s(call), 0));
}

char *to_string(SEXP var) {
    SEXP src = deparse1s(var);
    char *str = NULL;

    if (IS_SCALAR(src, STRSXP)) {
        str = strdup(CHAR(STRING_ELT(src, 0)));
    } else {
        str = strdup("<unsupported>");
    }

    return str;
}

int is_byte_compiled(SEXP op) {
    SEXP body = BODY(op);
    return TYPEOF(body) == BCODESXP;
}

const char *get_expression(SEXP e) {
    return CHAR(STRING_ELT(deparse1line(e, FALSE), 0));
}

// returns a monotonic timestamp in microseconds
uint64_t timestamp() {
    uint64_t t;
#ifdef __MACH__
    t = clock_gettime_nsec_np(CLOCK_MONOTONIC);
#else
    struct timespec ts;    
    clock_gettime(CLOCK_MONOTONIC, &ts);
    t = ts.tv_sec * 1e9 + ts.tv_nsec;
#endif
    return t;
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

void rdt_start(const rdt_handler *handler, const SEXP prom) {
    if (handler == NULL) {
        REprintf("RDT: rdt_start() called with NULL handler\n");
        return;
    }

    if (R_Verbose) {	
        REprintf("RDT: rdt_start(%p, ...)\n", handler);
    }

    rdt_curr_handler = handler;

    RDT_HOOK(probe_begin, prom);
}

void rdt_stop() {
    if (R_Verbose) {	
        REprintf("RDT: rdt_stop()\n");
    }

    RDT_HOOK(probe_end);

    rdt_curr_handler = &rdt_null_handler;
}

int rdt_is_running() {
    return rdt_curr_handler != &rdt_null_handler;
}
