#ifndef	_RDTRACE_H
#define	_RDTRACE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <Defn.h>

#define RDT_IS_ENABLED(name) (rdt_curr_handler->name != NULL)
#define RDT_FIRE_PROBE(name, ...) (rdt_curr_handler->name(__VA_ARGS__))

// ----------------------------------------------------------------------------
// probes
// ----------------------------------------------------------------------------
typedef struct rdt_handler {
    void (*probe_begin)();
    void (*probe_end)();
    void (*probe_function_entry)(const SEXP call, const SEXP op, const SEXP rho);
    void (*probe_function_exit)(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval);
    void (*probe_builtin_entry)(const SEXP call);
    void (*probe_builtin_exit)(const SEXP call, const SEXP retval);
    void (*probe_force_promise_entry)(const SEXP symbol, const SEXP rho);
    void (*probe_force_promise_exit)(const SEXP symbol, const SEXP val);
    void (*probe_promise_lookup)(const SEXP symbol, const SEXP val);
    void (*probe_error)(const SEXP call, const char* message);
    void (*probe_vector_alloc)(int sexptype, long length, long bytes, const char* srcref);
    void (*probe_eval_entry)(SEXP e, SEXP rho);
    void (*probe_eval_exit)(SEXP e, SEXP rho, SEXP retval);    
    void (*probe_gc_entry)(R_size_t size_needed);
    void (*probe_gc_exit)(int gc_count, double vcells, double ncells);
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


#endif	/* _RDTRACE_H */
