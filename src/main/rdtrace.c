#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rdtrace.h>
#include <time.h>

#if !defined(HAVE_CLOCK_GETTIME) && defined(__MACH__)
#include <mach/clock.h>
#include <mach/mach.h>
#endif

static const rdt_handler rdt_null_handler;

const rdt_handler *rdt_curr_handler = &rdt_null_handler;

//-----------------------------------------------------------------------------
// helpers
//-----------------------------------------------------------------------------

int gc_toggle_off() {
    int gc_enabled = R_GCEnabled;
    R_GCEnabled = 0;
    return gc_enabled;
}
void gc_toggle_restore(int previous_value) {
    R_GCEnabled = previous_value;
}

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
        case SPECIALSXP:
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

static int get_colno(SEXP srcref) {
    if (srcref && srcref != R_NilValue) {

        if (TYPEOF(srcref) == VECSXP) {
            srcref = VECTOR_ELT(srcref, 0);
        }

//        INTEGER(val)[0] = lloc->first_line;
//        INTEGER(val)[1] = lloc->first_byte;
//        INTEGER(val)[2] = lloc->last_line;
//        INTEGER(val)[3] = lloc->last_byte;
//        INTEGER(val)[4] = lloc->first_column;
//        INTEGER(val)[5] = lloc->last_column;
//        INTEGER(val)[6] = lloc->first_parsed;
//        INTEGER(val)[7] = lloc->last_parsed;

        if(TYPEOF(srcref) == INTSXP) {
            //lineno = INTEGER(srcref)[0];
            return INTEGER(srcref)[4];
        } else {
            // This should never happen, right?
            return -1;
        }
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

char *get_callsite(int how_far_in_the_past) {
    SEXP srcref = R_GetCurrentSrcref(how_far_in_the_past);
    const char *filename = get_filename(srcref);
    int lineno = get_lineno(srcref);
    int colno = get_colno(srcref);
    char *location = NULL;

    if (filename) {
        if (strlen(filename) > 0) {
            asprintf(&location, "%s:%d,%d", filename, lineno, colno);
        } else {
            asprintf(&location, "<console>:%d,%d", lineno, colno);
        }
    }

    return location;
}

char *get_location(SEXP op) {
    SEXP srcref = getAttrib(op, R_SrcrefSymbol);
    const char *filename = get_filename(srcref);
    int lineno = get_lineno(srcref);
    int colno = get_colno(srcref);
    char *location = NULL;

    if (filename) {
        if (strlen(filename) > 0) {
            asprintf(&location, "%s:%d,%d", filename, lineno, colno);
        } else {
            asprintf(&location, "<console>:%d,%d", lineno, colno);
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
    // The __MACH__ bit is from http://stackoverflow.com/a/6725161/6846474
#if !defined(HAVE_CLOCK_GETTIME) && defined(__MACH__)
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    t = mts.tv_sec * 1e9 + mts.tv_nsec;
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
