#ifndef	_RDTRACE_H
#define	_RDTRACE_H

#include <stdint.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// Had problems with multiple definitions of `R_OutputCon` which is a global int defined in Defn.h
//#include <Defn.h>

// Using Rinternals.h instead
#include <Rinternals.h>
typedef size_t R_size_t;
extern SEXP R_TrueValue;
extern SEXP R_FalseValue;
extern SEXP R_LogicalNAValue;

#define RDT_IS_ENABLED(name) (rdt_curr_handler->name != NULL)
#define RDT_FIRE_PROBE(name, ...) (rdt_curr_handler->name(__VA_ARGS__))

#ifdef ENABLE_RDT
#define RDT_HOOK(name, ...) \
    if(RDT_IS_ENABLED(name)) { \
        RDT_FIRE_PROBE(name, __VA_ARGS__); \
    }
#else
#define RDT_HOOK(name, ...)
#endif

// ----------------------------------------------------------------------------
// probes
// ----------------------------------------------------------------------------
typedef struct rdt_handler {
    void (*probe_begin)();
    void (*probe_end)();
    void (*probe_function_entry)(const SEXP call, const SEXP op, const SEXP rho);
    void (*probe_function_exit)(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval);
    void (*probe_builtin_entry)(const SEXP call, const SEXP op, const SEXP rho);
    void (*probe_builtin_exit)(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval);
    void (*probe_force_promise_entry)(const SEXP symbol, const SEXP rho);
    void (*probe_force_promise_exit)(const SEXP symbol, const SEXP rho, const SEXP val);
    void (*probe_promise_lookup)(const SEXP symbol, const SEXP rho, const SEXP val);
    void (*probe_error)(const SEXP call, const char* message);
    void (*probe_vector_alloc)(int sexptype, long length, long bytes, const char* srcref);
    void (*probe_eval_entry)(SEXP e, SEXP rho);
    void (*probe_eval_exit)(SEXP e, SEXP rho, SEXP retval);    
    void (*probe_gc_entry)(R_size_t size_needed);
    void (*probe_gc_exit)(int gc_count, double vcells, double ncells);
    void (*probe_gc_promise_unmarked)(const SEXP promise);
    void (*probe_jump_ctxt)(const SEXP rho, const SEXP val);
    void (*probe_S3_generic_entry)(const char *generic, const SEXP object);    
    void (*probe_S3_generic_exit)(const char *generic, const SEXP object, const SEXP retval);
    void (*probe_S3_dispatch_entry)(const char *generic, const char *clazz, const SEXP method, const SEXP object);
    void (*probe_S3_dispatch_exit)(const char *generic, const char *clazz, const SEXP method, const SEXP object, const SEXP retval);
} rdt_handler;

void rdt_start(const rdt_handler *handler);
void rdt_stop();
int rdt_is_running();

// the current handler
extern const rdt_handler *rdt_curr_handler;

// ----------------------------------------------------------------------------
// helpers
// ----------------------------------------------------------------------------

#define CHKSTR(s) ((s) == NULL ? "<unknown>" : (s))

#ifdef __cplusplus
extern "C" {
#endif

const char *get_ns_name(SEXP op);
const char *get_name(SEXP call);
char *get_location(SEXP op);
const char *get_call(SEXP call);
int is_byte_compiled(SEXP op);
char *to_string(SEXP var);
const char *get_expression(SEXP e);
SEXP get_named_list_element(const SEXP list, const char *name);

// Returns a monotonic timestamp in nanoseconds.
uint64_t timestamp();

#ifdef __cplusplus
}
#endif

#endif	/* _RDTRACE_H */
