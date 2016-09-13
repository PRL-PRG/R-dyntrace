#include "rdtrace.h"

typedef struct {
    const char *file_name;
    int line_no;
} source_code_loc;

static source_code_loc get_loc(SEXP srcref) {
    source_code_loc loc = { "unknown", 0 };

    if (srcref && srcref != R_NilValue) {
        if (TYPEOF(srcref) == VECSXP) srcref = VECTOR_ELT(srcref, 0);
        SEXP srcfile = getAttrib(srcref, R_SrcfileSymbol);
        if (TYPEOF(srcfile) == ENVSXP) {
            SEXP filename = findVar(install("filename"), srcfile);
            if (isString(filename) && length(filename)) {
                loc.file_name = CHAR(STRING_ELT(filename, 0));
                loc.line_no = asInteger(srcref);               
            }
        }
    }
    
    return loc;
}

void rdtrace_function_entry(SEXP call, SEXP op, SEXP rho) {
    const char *function_name = CHAR(CAAR(call));
    const source_code_loc loc = get_loc(R_Srcref);

    R_FUNCTION_ENTRY(function_name, loc.file_name, loc.line_no);
}
