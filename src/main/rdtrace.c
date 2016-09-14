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

void rdtrace_function_entry(SEXP call, SEXP op, SEXP rho) {
    const char *fun_name = CHAR(CAAR(call));
    const char *ns_name = get_ns_name(op);
    const char *filename = get_filename(R_Srcref);
    int lineno = get_lineno(R_Srcref);

    char *name = NULL;
    asprintf(&name, "%s%s%s", 
        ns_name ? ns_name : "",
        ns_name ? "::" : "",
        fun_name ? fun_name : "<unknown>");
    
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
    
    R_FUNCTION_ENTRY(name, location);

    free(name);
    free(location);
}
