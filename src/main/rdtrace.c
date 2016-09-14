#include "rdtrace.h"

static const char *get_ns_name(SEXP op) {
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

static char *get_fun_name(SEXP call, SEXP op) {
    const char *fun_name = CHAR(CAAR(call));
    const char *ns_name = get_ns_name(op);
    char *name = NULL;
    
    asprintf(&name, "%s%s%s", 
        ns_name ? ns_name : "",
        ns_name ? "::" : "",
        fun_name ? fun_name : "<unknown>");
    
    return name;
}

static char *get_location(SEXP srcref) {
    const char *filename = get_filename(srcref);
    int lineno = get_lineno(srcref);
    char *location = NULL;

    if (filename) {
        if (strlen(filename) > 0) {
            asprintf(&location, "%s:%d", filename, lineno);
        } else {
            location = strdup("<console>");
        }
    } else {
        location = strdup("<unknown>");
    }

    return location;
}

static char *to_string(SEXP var) {
    return strdup("");
}

static int get_flags(SEXP op) {
    int flags = FUN_NO_FLAGS;

    SEXP body = BODY(op);
    flags = TYPEOF(body) == BCODESXP ? flags | FUN_BC : flags;

    return flags;
}

void rdtrace_function_entry(SEXP call, SEXP op, SEXP rho) {
    char *name = get_fun_name(call, op);
    SEXP srcref = getAttrib(op, R_SrcrefSymbol);
    char *location = get_location(srcref);
    int flags = get_flags(op);

    R_FUNCTION_ENTRY(name, location, flags);

    free(name);
    free(location);
}

void rdtrace_function_exit(SEXP call, SEXP op, SEXP rho, SEXP rv) {
    char *name = get_fun_name(call, op);
    SEXP srcref = getAttrib(op, R_SrcrefSymbol);
    char *location = get_location(srcref);
    char *retval = to_string(rv);
    int flags = get_flags(op);

    R_FUNCTION_EXIT(name, location, flags, TYPEOF(rv), "");

    free(name);
    free(location);
    free(retval); 
}

void rdtrace_force_promise_entry(SEXP symbol) {
    const char *name = CHAR(PRINTNAME(symbol));

    R_FORCE_PROMISE_ENTRY(name);
}

void rdtrace_force_promise_exit(SEXP symbol, SEXP val) {
    const char *name = CHAR(PRINTNAME(symbol));

    R_FORCE_PROMISE_EXIT(name, TYPEOF(val), "");
}
