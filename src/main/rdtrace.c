#include <errno.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <rdtrace.h>

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
// probes enables checkers
//-----------------------------------------------------------------------------

int rdt_probe_begin_enabled() {
	return rdt_probe_begin_ref != NULL;
}

int rdt_probe_end_enabled() {
	return rdt_probe_end_ref != NULL;
}

int rdt_probe_function_entry_enabled() {
	return rdt_probe_function_entry_ref != NULL;
}

int rdt_probe_function_exit_enabled() {
	return rdt_probe_function_exit_ref != NULL;
}

int rdt_probe_builtin_entry_enabled() {
	return rdt_probe_builtin_entry_ref != NULL;
}

int rdt_probe_builtin_exit_enabled() {
	return rdt_probe_builtin_exit_ref != NULL;
}

int rdt_probe_force_promise_entry_enabled() {
	return rdt_probe_force_promise_entry_ref != NULL;
}

int rdt_probe_force_promise_exit_enabled() {
	return rdt_probe_force_promise_exit_ref != NULL;
}

int rdt_probe_promise_lookup_enabled() {
	return rdt_probe_promise_lookup_ref != NULL;
}

int rdt_probe_error_enabled() {
	return rdt_probe_error_ref != NULL;
}

int rdt_probe_vector_alloc_enabled() {
	return rdt_probe_vector_alloc_ref != NULL;
}

int rdt_probe_eval_entry_enabled() {
	return rdt_probe_eval_entry_ref != NULL;
}

int rdt_probe_eval_exit_enabled() {
	return rdt_probe_eval_exit_ref != NULL;
}

//-----------------------------------------------------------------------------
// probes launchers
//-----------------------------------------------------------------------------

void rdt_probe_begin() {
	(*rdt_probe_begin_ref)();
}

void rdt_probe_end() {
	(*rdt_probe_end_ref)();
}

void rdt_probe_function_entry(const SEXP call, const SEXP op, const SEXP rho) {
	(*rdt_probe_function_entry_ref)(call, op, rho);
}

void rdt_probe_function_exit(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
	(*rdt_probe_function_exit_ref)(call, op, rho, retval);
}

void rdt_probe_builtin_entry(const SEXP call) {
	(*rdt_probe_builtin_entry_ref)(call);
}

void rdt_probe_builtin_exit(const SEXP call, const SEXP retval) {
	(*rdt_probe_builtin_exit_ref)(call, retval);
}

void rdt_probe_force_promise_entry(const SEXP symbol) {
	(*rdt_probe_force_promise_entry_ref)(symbol);
}

void rdt_probe_force_promise_exit(const SEXP symbol, const SEXP val) {
	(*rdt_probe_force_promise_exit_ref)(symbol, val);
}

void rdt_probe_promise_lookup(const SEXP symbol, const SEXP val) {
	(*rdt_probe_promise_lookup_ref)(symbol, val);
}

void rdt_probe_error(const SEXP call, const char* message) {
	(*rdt_probe_error_ref)(call, message);
}

void rdt_probe_vector_alloc(int sexptype, long length, long bytes, const char *srcref) {
	(*rdt_probe_vector_alloc_ref)(sexptype, length, bytes,  srcref);
}

void rdt_probe_eval_entry(SEXP e, SEXP rho) {
	(*rdt_probe_eval_entry_ref)(e, rho);
}

void rdt_probe_eval_exit(SEXP e, SEXP rho, SEXP retval) {
	(*rdt_probe_eval_exit_ref)(e, rho, retval);
}

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

const char *get_fun_name(SEXP call) {
    const char *s = NULL;

    switch(TYPEOF(call)) {
        case LANGSXP:
            s = CHAR(CAAR(call));
            break;
        case BUILTINSXP:
            s = PRIMNAME(call);
            break;
        default:
            s = "<unknown>";
    }

    return s;
}

char *get_fun_fq_name(SEXP call, SEXP op) {
    const char *ns_name = get_ns_name(op);
    const char *fun_name = get_fun_name(call);
    char *name = NULL;
    
    asprintf(&name, "%s%s%s", 
        ns_name ? ns_name : "",
        ns_name ? "::" : "",
        fun_name ? fun_name : "<unknown>");
    
    return name;
}

const char *get_symbol_name(SEXP symbol) {
    return CHAR(PRINTNAME(symbol));
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
    } else {
        location = strdup("<unknown>");
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

static void *rdt_handler = NULL; 

void rdt_start() {
    char *rdt_handler_path = getenv(R_DT_HANDLER);
    if (! rdt_handler_path) {
        if(R_Verbose) {
            Rprintf("Environment variable %s is not defined - RDT is disabled\n", R_DT_HANDLER);
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
        
#define LOAD_CALLBACK(s) rdt_##s##_ref = load_rdt_callback(rdt_handler, #s)
    LOAD_CALLBACK(probe_begin);
    LOAD_CALLBACK(probe_end);
    LOAD_CALLBACK(probe_function_entry);
    LOAD_CALLBACK(probe_function_exit);
    LOAD_CALLBACK(probe_builtin_entry);
    LOAD_CALLBACK(probe_builtin_exit);
    LOAD_CALLBACK(probe_force_promise_entry);
    LOAD_CALLBACK(probe_force_promise_exit);
    LOAD_CALLBACK(probe_promise_lookup);
    LOAD_CALLBACK(probe_error);
    LOAD_CALLBACK(probe_vector_alloc);
    LOAD_CALLBACK(probe_eval_entry);
    LOAD_CALLBACK(probe_eval_exit);
#undef LOAD_CALLBACK

    if (rdt_probe_begin_enabled()) {
        rdt_probe_begin();
    }     
}

void rdt_stop() {
    if (rdt_probe_end_enabled()) {
        rdt_probe_end();
    }     

    if (rdt_handler) {
        dlclose(rdt_handler);
        rdt_handler = NULL;
    }
}

// void rdtrace_eval_expression(SEXP expression, SEXP rho) {
//     // const char *s = 
    
//     // R_EVAL_EXPRESSION(TYPEOF(expression), s);
// }

// void rdtrace_eval_return(SEXP rv, SEXP rho) {
//     // const char *s = CHAR(STRING_ELT(deparse1line(rv, FALSE), 0));

//     // R_EVAL_RETURN(TYPEOF(rv), s);
// }