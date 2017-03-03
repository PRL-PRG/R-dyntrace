#include <vector>
#include <string>
#include <stack>
#include <unordered_map>
#include <utility>

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
//#include <Defn.h>

extern "C" {
#include "../../../main/inspect.h"
}

#include "rdt.h"

using namespace std;

// If defined printout will include increasing indents showing function calls.
#define RDT_PROMISES_INDENT
#define TAB_WIDTH 4

// Use generated call IDs instead of function env addresses
#define RDT_CALL_ID

typedef uintptr_t rid_t;

#define RID_INVALID -1

static FILE *output = NULL;
static uint64_t last = 0;
static uint64_t delta = 0;

static int indent;
static int call_id_counter;

// Function call stack (may be useful)
// Whenever R makes a function call, we generate a function ID and store that ID on top of the stack
// so that we know where we are (e.g. when printing function ID at function_exit hook)

static stack<rid_t, vector<rid_t>> fun_stack;

#ifdef RDT_CALL_ID
#define CALL_ID_FMT "%d"
static stack<rid_t, vector<rid_t>> curr_env_stack;
#else
#define CALL_ID_FMT "%#x"
#endif

// Map from promise IDs to call IDs
static unordered_map<rid_t, rid_t> promise_origin;

typedef vector<rid_t> prom_vec_t;
typedef pair<string, prom_vec_t> arg_t;


static inline void print_builtin(const char *type, const char *loc, const char *name, rid_t id) {
    fprintf(output,
#ifdef RDT_PROMISES_INDENT
            "%*s%s loc(%s) function(%s=%#x)\n",
            indent,
            "",
#else
            "%s loc(%s) function(%s=%#x)\n",
#endif
            //delta,
            type,
            CHKSTR(loc),
            CHKSTR(name),
            id);
}

static inline void print_promise(const char *type, const char *loc, const char *name, rid_t id, rid_t in_call_id, rid_t from_call_id) {
    fprintf(output,
#ifdef RDT_PROMISES_INDENT
            "%*s%s loc(%s) promise(%s=%#x) in_call(" CALL_ID_FMT ") from_call(" CALL_ID_FMT ")\n",
            indent,
            "",
#else
            "%s loc(%s) promise(%s=%#x) in_call(" CALL_ID_FMT ") from_call(" CALL_ID_FMT ")\n",
#endif
            //delta,
            type,
            CHKSTR(loc),
            CHKSTR(name),
            id,
            in_call_id,
            from_call_id);
}

static inline void print_function(const char *type, const char *loc, const char *name, rid_t function_id, rid_t call_id, vector<arg_t> const& arguments, const int arguments_num) {
    fprintf(output,
#ifdef RDT_PROMISES_INDENT
        "%*s%s loc(%s) call(" CALL_ID_FMT ") function(%s=%#x) ",
        indent, // http://stackoverflow.com/a/9448093/6846474
        "",
#else
        "%s loc(%s) call(" CALL_ID_FMT ") function(%s=%#x) ",
#endif
        type,
        CHKSTR(loc),
        call_id,
        CHKSTR(name),
        function_id
    );

    // print argument names and the promises bound to them
    int i = 0;
    fprintf(output, "arguments(");
    for (auto & a : arguments) {
        const prom_vec_t & p = a.second;
        fprintf(output, "%s=%#x", a.first.c_str(), p[0]);

        // iterate from second bound value if there is any
        // and produce comma separated list of promises
        for (auto it = ++p.begin(); it != p.end(); it++) {
            fprintf(output, ",%#x", *it);
        }
        if (i < arguments_num - 1) fprintf(output, ";"); // semicolon separates individual args
        ++i;
    }

    fprintf(output, ")\n");
}

static inline void print_unwind(const char *type, rid_t call_id) {
    fprintf(output,
#ifdef RDT_PROMISES_INDENT
            "%*s%s unwind call(" CALL_ID_FMT ")\n",
            indent,
            "",
#else
            "%s unwind call(" CALL_ID_FMT ")\n",
#endif
            type,
            call_id
    );
}

// TODO remove
static inline void compute_delta() {
    delta = (timestamp() - last) / 1000;
}

// ??? can we get metadata about the program we're analysing in here?
static void trace_promises_begin() {
    indent = 0;
    call_id_counter = 0;

    output = fopen("trace.txt", "w");

    //fprintf(output, "TYPE,LOCATION,NAME\n");
    //fflush(output);

    // We have to make sure the stack is not empty
    // when referring to the promise created by call to Rdt.
    // This is just a dummy call and environment.
    fun_stack.push(0);
#ifdef RDT_CALL_ID
    curr_env_stack.push(0);
#endif

    last = timestamp();
}
static void trace_promises_end() {
    fun_stack.pop();
#ifdef RDT_CALL_ID
    curr_env_stack.pop();
#endif

    promise_origin.clear();
    fclose(output);
}

static inline int count_elements(SEXP list) {
    int counter = 0;
    SEXP tmp = list;
    for (; tmp != R_NilValue; counter++)
        tmp = CDR(tmp);
    return counter;
}

static inline rid_t get_sexp_address(SEXP e) {
    return (rid_t)e;
}

static inline rid_t get_promise_id(SEXP promise) {
    if (promise == R_NilValue)
        return RID_INVALID;
    if (TYPEOF(promise) != PROMSXP)
        return RID_INVALID;

    // A new promise is always created for each argument.
    // Even if the argument is already a promise passed from the caller, it gets re-wrapped.
    return get_sexp_address(promise);
}

static inline rid_t get_function_id(SEXP func) {
    assert(TYPEOF(func) == CLOSXP);
    return get_sexp_address(func);
}

#ifdef RDT_CALL_ID
static inline rid_t make_funcall_id(SEXP function) {
    if (function == R_NilValue)
        return RID_INVALID;

    return ++call_id_counter;
}
#else
static inline rid_t make_funcall_id(SEXP fn_env) {
    assert(fn_env != NULL);
    return get_sexp_address(fn_env);
}
#endif

// When doing longjump (exception thrown, etc.) this function gets the target environment
// and unwinds function call stack until that environment is on top. It also fixes indentation.
static inline void adjust_fun_stack(SEXP rho) {
    rid_t call_id, call_addr;

    while (!fun_stack.empty() &&
#ifdef RDT_CALL_ID
            (call_addr = curr_env_stack.top()) && get_sexp_address(rho) != call_addr
#else
            (call_id = fun_stack.top()) && get_sexp_address(rho) != call_id
#endif
            ) {
#ifdef RDT_CALL_ID
        call_id = fun_stack.top();
        curr_env_stack.pop();
#endif
        fun_stack.pop();
#ifdef RDT_PROMISES_INDENT
        indent -= TAB_WIDTH;
#endif
        print_unwind("<=", call_id);
    }
}

// Wraper for findVar. Does not look up the value if it already is PROMSXP.
static SEXP get_promise(SEXP var, SEXP rho) {
    SEXP prom = R_NilValue;

    if (TYPEOF(var) == PROMSXP) {
        prom = var;
    } else if (TYPEOF(var) == SYMSXP) {
        prom = findVar(var, rho);
    }

    return prom;
}

static inline int get_arguments(SEXP op, SEXP rho, vector<arg_t> & arguments) {
    SEXP formals = FORMALS(op);

    int argument_count = count_elements(formals);

    for (int i=0; i<argument_count; i++, formals = CDR(formals)) {
        // Retrieve the argument name.
        SEXP argument_expression = TAG(formals);

        arg_t arg;
        string & arg_name = arg.first;
        prom_vec_t & arg_prom_vec = arg.second;

        arg_name = get_name(argument_expression);

        // Retrieve the promise for the argument.
        // The call SEXP only contains AST to find the actual argument value, we need to search the environment.
        SEXP promise_expression = get_promise(argument_expression, rho);

        if (TYPEOF(promise_expression) == DOTSXP) {
            for(SEXP dots = promise_expression; dots != R_NilValue; dots = CDR(dots)) {
                arg_prom_vec.push_back(get_promise_id(CAR(dots)));
            }
        }
        else {
            arg_prom_vec.push_back(get_promise_id(promise_expression));
        }
        arguments.push_back(arg);
    }

    return argument_count;
}


// Triggered when entering function evaluation.
static void trace_promises_function_entry(const SEXP call, const SEXP op, const SEXP rho) {
    compute_delta();

    const char *type = is_byte_compiled(call) ? "=> bcod" : "=> func";
    const char *name = get_name(call);
    const char *ns = get_ns_name(op);
    rid_t fn_id = get_function_id(op);
#ifdef RDT_CALL_ID
    rid_t call_id = make_funcall_id(op);
#else
    rid_t call_id = make_funcall_id(rho);
#endif
    char *loc = get_location(op);
    char *fqfn = NULL;

    if (ns) {
        asprintf(&fqfn, "%s::%s", ns, CHKSTR(name));
    } else {
        fqfn = name != NULL ? strdup(name) : NULL;
    }

    // Push function ID on function stack
    fun_stack.push(call_id);
#ifdef RDT_CALL_ID
    curr_env_stack.push(get_sexp_address(rho));
#endif

    vector<arg_t> arguments;
    int argument_count;

    argument_count = get_arguments(op, rho, arguments);
    print_function(type, loc, fqfn, fn_id, call_id, arguments, argument_count);

    #ifdef RDT_PROMISES_INDENT
    indent += TAB_WIDTH;
    #endif

    // Associate promises with call ID
    for (auto & a : arguments) {
        auto & promises = a.second;
        for (auto p : promises) {
            promise_origin[p] = call_id;
        }
    }

    if (loc)
        free(loc);
    if (fqfn)
        free(fqfn);

    last = timestamp();
}

static void trace_promises_function_exit(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
    compute_delta();

    #ifdef RDT_PROMISES_INDENT
    indent -= TAB_WIDTH;
    #endif

    const char *type = is_byte_compiled(call) ? "<= bcod" : "<= func";
    const char *name = get_name(call);
    const char *ns = get_ns_name(op);
    rid_t fn_id = get_function_id(op);
    rid_t call_id = fun_stack.top();
    char *loc = get_location(op);
    char *fqfn = NULL;

    if (ns) {
        asprintf(&fqfn, "%s::%s", ns, CHKSTR(name));
    } else {
        fqfn = name != NULL ? strdup(name) : NULL;
    }

    vector<arg_t> arguments;
    int argument_count;

    argument_count = get_arguments(op, rho, arguments);
    print_function(type, loc, name, fn_id, call_id, arguments, argument_count);

    // Pop current function ID
    fun_stack.pop();
#ifdef RDT_CALL_ID
    curr_env_stack.pop();
#endif

    if (loc)
        free(loc);
    if (fqfn)
        free(fqfn);

    last = timestamp();
}

// XXX Probably don't need this?
static void trace_promises_builtin_entry(const SEXP call, const SEXP op, const SEXP rho) {
    compute_delta();

    const char *name = get_name(call);
    rid_t id = get_function_id(op);

    print_builtin("=> b-in", NULL, name, id);

    last = timestamp();
}

static void trace_promises_builtin_exit(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
    compute_delta();

    const char *name = get_name(call);
    rid_t id = get_function_id(op);

    print_builtin("<= b-in", NULL, name, id);

    last = timestamp();
}

// Promise is being used inside a function body for the first time.
static void trace_promises_force_promise_entry(const SEXP symbol, const SEXP rho) {
    compute_delta();

    const char *name = get_name(symbol);

    SEXP promise_expression = get_promise(symbol, rho);
    rid_t id = get_promise_id(promise_expression);
    rid_t in_call_id = fun_stack.top();
    rid_t from_call_id = promise_origin[id];

    print_promise("=> prom", NULL, name, id, in_call_id, from_call_id);

    last = timestamp();
}

static void trace_promises_force_promise_exit(const SEXP symbol, const SEXP rho, const SEXP val) {
    compute_delta();

    const char *name = get_name(symbol);

    SEXP promise_expression = get_promise(symbol, rho);
    rid_t id = get_promise_id(promise_expression);
    rid_t in_call_id = fun_stack.top();
    rid_t from_call_id = promise_origin[id];

    print_promise("<= prom", NULL, name, id, in_call_id, from_call_id);

    last = timestamp();
}

static void trace_promises_promise_lookup(const SEXP symbol, const SEXP rho, const SEXP val) {
    compute_delta();

    const char *name = get_name(symbol);

    SEXP promise_expression = get_promise(symbol, rho);
    rid_t id = get_promise_id(promise_expression);
    rid_t in_call_id = fun_stack.top();
    rid_t from_call_id = promise_origin[id];

    print_promise("<> lkup", NULL, name, id, in_call_id, from_call_id);

    last = timestamp();
}

static void trace_promises_error(const SEXP call, const char* message) {
    compute_delta();

    char *call_str = NULL;
    char *loc = get_location(call);

    asprintf(&call_str, "\"%s\"", get_call(call));

    if (loc) free(loc);
    if (call_str) free(call_str);

    last = timestamp();
}

static void trace_promises_vector_alloc(int sexptype, long length, long bytes, const char* srcref) {
    compute_delta();

    last = timestamp();
}

// static void trace_eval_entry(SEXP e, SEXP rho) {
//     switch(TYPEOF(e)) {
//         case LANGSXP:
//             fprintf(output, "%s\n");
//             PrintValue
//         break;
//     }
// }

// static void trace_eval_exit(SEXP e, SEXP rho, SEXP retval) {
//     printf("");
// }

static void trace_promises_gc_promise_unmarked(const SEXP promise) {
    rid_t id = (rid_t)promise;

    auto iter = promise_origin.find(id);
    if (iter != promise_origin.end()) {
        // If this is one of our traced promises,
        // delete it from origin map because it is ready to be GCed
        promise_origin.erase(iter);
        //Rprintf("Promise %#x deleted.\n", id);
    }
}

static void trace_promises_jump_ctxt(const SEXP rho, const SEXP val) {
    adjust_fun_stack(rho);
}

static void trace_promises_gc_entry(R_size_t size_needed) {
    compute_delta();

    last = timestamp();
}

static void trace_promises_gc_exit(int gc_count, double vcells, double ncells) {
    compute_delta();

    last = timestamp();
}

static void trace_promises_S3_generic_entry(const char *generic, const SEXP object) {
    compute_delta();

    last = timestamp();
}

static void trace_promises_S3_generic_exit(const char *generic, const SEXP object, const SEXP retval) {
    compute_delta();

    last = timestamp();
}

static void trace_promises_S3_dispatch_entry(const char *generic, const char *clazz, const SEXP method, const SEXP object) {
    compute_delta();

    last = timestamp();
}

static void trace_promises_S3_dispatch_exit(const char *generic, const char *clazz, const SEXP method, const SEXP object, const SEXP retval) {
    compute_delta();

    last = timestamp();
}


// TODO properly turn off probes we don't use
static const rdt_handler trace_promises_rdt_handler = {
        &trace_promises_begin,
        &trace_promises_end,
        &trace_promises_function_entry,
        &trace_promises_function_exit,
        &trace_promises_builtin_entry,
        &trace_promises_builtin_exit,
        &trace_promises_force_promise_entry,
        &trace_promises_force_promise_exit,
        &trace_promises_promise_lookup,
        &trace_promises_error,
        &trace_promises_vector_alloc,
        NULL, // &trace_eval_entry,
        NULL, // &trace_eval_exit,
        &trace_promises_gc_entry,
        &trace_promises_gc_exit,
        &trace_promises_gc_promise_unmarked,
        &trace_promises_jump_ctxt,
        &trace_promises_S3_generic_entry,
        &trace_promises_S3_generic_exit,
        &trace_promises_S3_dispatch_entry,
        &trace_promises_S3_dispatch_exit
};

rdt_handler *setup_promise_tracing(SEXP options) {
    const char *filename = get_string(get_named_list_element(options, "filename"));
    output = filename != NULL ? fopen(filename, "wt") : stderr;

    if (!output) {
        error("Unable to open %s: %s\n", filename, strerror(errno));
        return NULL;
    }

    rdt_handler *h = (rdt_handler *)  malloc(sizeof(rdt_handler));
    memcpy(h, &trace_promises_rdt_handler, sizeof(rdt_handler));

    SEXP disabled_probes = get_named_list_element(options, "disabled.probes");
    if (disabled_probes != R_NilValue && TYPEOF(disabled_probes) == STRSXP) {
        for (int i=0; i<LENGTH(disabled_probes); i++) {
            const char *probe = CHAR(STRING_ELT(disabled_probes, i));

            if (!strcmp("function", probe)) {
                h->probe_function_entry = NULL;
                h->probe_function_exit = NULL;
            } else if (!strcmp("builtin", probe)) {
                h->probe_builtin_entry = NULL;
                h->probe_builtin_exit = NULL;
            } else if (!strcmp("promise", probe)) {
                h->probe_promise_lookup = NULL;
                h->probe_force_promise_entry = NULL;
                h->probe_force_promise_exit = NULL;
            } else if (!strcmp("vector", probe)) {
                h->probe_vector_alloc = NULL;
            } else if (!strcmp("gc", probe)) {
                h->probe_gc_entry = NULL;
                h->probe_gc_exit = NULL;
            } else if (!strcmp("S3", probe)) {
                h->probe_S3_dispatch_entry = NULL;
                h->probe_S3_dispatch_exit = NULL;
                h->probe_S3_generic_entry = NULL;
                h->probe_S3_generic_exit = NULL;
            } else {
                warning("Unknown probe `%s`\n", probe);
            }
        }
    }

    last = 0;
    delta = 0;

    return h;
}
