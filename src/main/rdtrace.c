#include <errno.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <rdtrace.h>
#include <time.h>

#define LOAD_PROBE(name) rdt_##name##_ref = load_rdt_callback(rdt_handler, #name)

//-----------------------------------------------------------------------------
// probes refererences
//-----------------------------------------------------------------------------

void (*rdt_probe_begin_ref)();
void (*rdt_probe_end_ref)();
void (*rdt_probe_function_entry_ref)(const SEXP call, const SEXP op, const SEXP rho);
void (*rdt_probe_function_exit_ref)(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval);
void (*rdt_probe_builtin_entry_ref)(const SEXP call);
void (*rdt_probe_builtin_exit_ref)(const SEXP call, const SEXP retval);
void (*rdt_probe_force_promise_entry_ref)(const SEXP symbol);
void (*rdt_probe_force_promise_exit_ref)(const SEXP symbol, const SEXP val);
void (*rdt_probe_promise_lookup_ref)(const SEXP symbol, const SEXP val);
void (*rdt_probe_error_ref)(const SEXP call, const char* message);
void (*rdt_probe_vector_alloc_ref)(int sexptype, long length, long bytes, const char* srcref);
void (*rdt_probe_eval_entry_ref)(SEXP e, SEXP rho);
void (*rdt_probe_eval_exit_ref)(SEXP e, SEXP rho, SEXP retval);

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
        case LANGSXP:
            s = CHAR(CAAR(sexp));
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

static void *load_rdt_callback(void *handle, const char *name) {
    dlerror();

    void *callback = dlsym(handle, name);

    if(R_Verbose) {	
        if (callback == NULL) {
            Rprintf("Probe `%s` not loaded: %s\n", name, dlerror());
        } else {
            Rprintf("Probe `%s` loaded\n", name);
        }
    }

    return callback;
}

// returns a monotonic timestamp in microseconds
uint64_t timestamp() {
    uint64_t t;
#ifdef __MACH__
    t = clock_gettime_nsec_np(CLOCK_MONOTONIC);
#else
    struct timespec ts;
    uint64_t
    clock_gettime(CLOCK_MONOTONIC, &ts);
    t = ts.tv_sec * 1e9 + ts.tv_nsec;
#endif
    return t;
}

static void *rdt_handler = NULL; 

void rdt_start() {
    char *rdt_handler_path = getenv(RDT_HANDLER_ENV_VAR);
    if (! rdt_handler_path) {
        if(R_Verbose) {
            Rprintf("Environment variable %s is not defined - RDT is disabled\n", RDT_HANDLER_ENV_VAR);
        }	
        return;
    }

    if(R_Verbose) {
        Rprintf("RDT handler: %s\n", rdt_handler_path);
    }	

    // load the handler with all the costs at startup
    rdt_handler = dlopen(rdt_handler_path, RTLD_LOCAL | RTLD_NOW);
    if (!rdt_handler) {
        REprintf("RDT: Unable to open R DT handler `%s`: %s\n", rdt_handler_path, dlerror());
        return;
    }
        
    LOAD_PROBE(probe_begin);
    LOAD_PROBE(probe_end);
    LOAD_PROBE(probe_function_entry);
    LOAD_PROBE(probe_function_exit);
    LOAD_PROBE(probe_builtin_entry);
    LOAD_PROBE(probe_builtin_exit);
    LOAD_PROBE(probe_force_promise_entry);
    LOAD_PROBE(probe_force_promise_exit);
    LOAD_PROBE(probe_promise_lookup);
    LOAD_PROBE(probe_error);
    LOAD_PROBE(probe_vector_alloc);
    LOAD_PROBE(probe_eval_entry);
    LOAD_PROBE(probe_eval_exit);

    if (RDT_IS_ENABLED(probe_begin)) {
        RDT_FIRE_PROBE(probe_begin);
    }     
}

void rdt_stop() {
    if (RDT_IS_ENABLED(probe_end)) {
        RDT_FIRE_PROBE(probe_end);
    }     

    if (rdt_handler) {
        dlclose(rdt_handler);
        rdt_handler = NULL;
    }
}