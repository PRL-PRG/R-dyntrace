#include "rdtrace.h"

#define max(a, b) ((a > b)?(a):(b))

Rboolean rdtrace_in_eval = FALSE;

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

static const char *get_fun_name(SEXP call) {
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

// get a fully qualified function name (<package>::<function>)
static char *get_fqfn(SEXP call, SEXP op) {
    const char *fun_name = get_fun_name(call);
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
    SEXP src = deparse1s(var);
    char *str = NULL;

    if (IS_SCALAR(src, STRSXP)) {
        str = strdup(CHAR(STRING_ELT(src, 0)));
    } else {
        str = strdup("<unsupported>");
    }

    return str;
}

static int get_flags(SEXP op) {
    int flags = FUN_NO_FLAGS;

    SEXP body = BODY(op);
    flags = TYPEOF(body) == BCODESXP ? flags | FUN_BC : flags;

    return flags;
}

static long v_size(long n, int size) {
  if (n == 0) return 8;

  double vec_size = max(sizeof(SEXP), sizeof(double));
  double elements_per_byte = vec_size / size;
  double n_bytes = ceil(n / elements_per_byte);

  long bytes = 0;
  // Big vectors always allocated in 8 byte chunks
  if (n_bytes > 16) bytes = n_bytes * 8;
  // For small vectors, round to sizes allocated in small vector pool
  else if (n_bytes > 8)  bytes = 128;
  else if (n_bytes > 6)  bytes = 64;
  else if (n_bytes > 4)  bytes = 48;
  else if (n_bytes > 2)  bytes = 32;
  else if (n_bytes > 1)  bytes = 16;
  else if (n_bytes > 0)  bytes = 8;

  return bytes + vec_size; // SEXPREC_ALIGN padding
}

static long object_size(SEXP x) {
  if (TYPEOF(x) == NILSXP ||
      TYPEOF(x) == SPECIALSXP ||
      TYPEOF(x) == BUILTINSXP) return 0;

  // As of R 3.1.0, all SEXPRECs start with sxpinfo (4 bytes, but aligned),
  // followed by three pointers: attribute pairlist, next object, prev object
  // (i.e. doubly linked list of all objects in memory)
  int ptr_size = sizeof(void*);
  long size = 4 * ptr_size;

  switch (TYPEOF(x)) {
    case LGLSXP:
    case INTSXP:
      size += v_size(XLENGTH(x), sizeof(int));
      break;
    case REALSXP:
      size += v_size(XLENGTH(x), sizeof(double));
      break;
    case CPLXSXP:
      size += v_size(XLENGTH(x), sizeof(Rcomplex));
      break;
    case RAWSXP:
      size += v_size(XLENGTH(x), 1);
      break;
    case STRSXP:
      size += v_size(XLENGTH(x), ptr_size);
      break;
    case CHARSXP:
      size += v_size(LENGTH(x) + 1, 1);
      break;
    case VECSXP:
    case EXPRSXP:
    case WEAKREFSXP:
      size += v_size(XLENGTH(x), sizeof(SEXP));
      break;
    case LISTSXP:
    case LANGSXP:      
      for(SEXP c = x; c != R_NilValue; c = CDR(c)) size += sizeof(SEXP); 
    //   size += 3 * sizeof(SEXP); // tag, car, cdr
      break;
    // case ENVSXP:
    //   if (x == R_BaseEnv || x == R_GlobalEnv || x == R_EmptyEnv || x == base_env || is_namespace(x)) 
    //     size = 0;
    //   else
    //     size += 3 * sizeof(SEXP); // frame, enclos, hashtab
    //   break;
    // case CLOSXP:
    //   size += 3 * sizeof(SEXP); // formals, body, env
    //   break;
    // case PROMSXP:
    //   size += 3 * sizeof(SEXP); // value, expr, env
    // case EXTPTRSXP:
    //   size += sizeof(void *); // the actual pointer
    //   break;
    // case S4SXP:
    //   break;
    // case SYMSXP:
    //   size += 3 * sizeof(SEXP); // pname, value, internal
    //   break;
    default:
        size = -1;
      break;
  }

  // Rcout << "type: " << TYPEOF(x) << " size: " << size << "\n";
  return size;
}

void rdtrace_function_entry(SEXP call, SEXP op, SEXP rho) {
    char *name = get_fqfn(call, op);
    SEXP srcref = getAttrib(op, R_SrcrefSymbol);
    char *location = get_location(srcref);
    int flags = get_flags(op);

    R_FUNCTION_ENTRY(name, location, flags);

    free(name);
    free(location);
}

void rdtrace_function_exit(SEXP call, SEXP op, SEXP rho, SEXP rv) {
    char *name = get_fqfn(call, op);
    SEXP srcref = getAttrib(op, R_SrcrefSymbol);
    char *location = get_location(srcref);
    char *retval = to_string(rv);
    int flags = get_flags(op);

    R_FUNCTION_EXIT(name, location, flags, TYPEOF(rv), retval);

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
    char *retval = to_string(val);

    R_FORCE_PROMISE_EXIT(name, TYPEOF(val), retval);

    free(retval); 
}

void rdtrace_promise_lookup(SEXP symbol, SEXP val) {
    const char *name = CHAR(PRINTNAME(symbol));
    char *retval = to_string(val);

    R_PROMISE_LOOKUP(name, TYPEOF(val), retval);

    free(retval);     
}

void rdtrace_builtin_entry(SEXP call) {
    const char *name = get_fun_name(call);

    R_BUILTIN_ENTRY(name);
}

void rdtrace_builtin_exit(SEXP call, SEXP rv) {
    const char *name = get_fun_name(call);
    char *retval = to_string(rv);

    R_BUILTIN_EXIT(name, TYPEOF(rv), retval);

    free(retval);
}

void rdtrace_error(SEXP call, const char *message) {
    const char *dcall = CHAR(STRING_ELT(deparse1s(call), 0));
	// SEXP srcloc = GetSrcLoc(R_GetCurrentSrcref(skip));
    // const char *location = CHAR(STRING_ELT(srcloc, 0));
    
    R_ERROR(dcall, "<unknown>", message);
}

void rdtrace_alloc_entry(int type, SEXPTYPE sexptype, long length) {
    R_ALLOC_ENTRY(type, sexptype, length);
}

void rdtrace_alloc_exit(int type, SEXPTYPE sexptype, long length, SEXP var) {
    long size = object_size(var);
    R_ALLOC_EXIT(type, sexptype, length, size);
}