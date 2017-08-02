#ifndef	_RDTRACE_H
#define	_RDTRACE_H

#include <stdint.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <Defn.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#define REG_HOOKS_BEGIN(handler_struct_ptr, tracer_name) \
    do { \
        typedef tracer_name tr; \
        rdt_handler * hnd = handler_struct_ptr; \
        memset(h, 0, sizeof(*h)) \

#define ADD_HOOK(hook_name) \
    hnd->probe_##hook_name = tr::hook_name \

#define REG_HOOKS_END } while(0)

// ----------------------------------------------------------------------------
// probes
// ----------------------------------------------------------------------------
typedef struct rdt_handler {
    // Fires when the tracer starts.
    // Not an interpreter hook. It is called from
    // src/main/rdtrace.c:rdt_start() which is called from src/library/rdt/src/rdt.c:Rdt() (the entrypoint)
    void (*probe_begin)(const SEXP prom);
    // Fires when the tracer ends (after the traced call returns)
    // Not an interpreter hook. It is called from
    // src/main/rdtrace.c:rdt_stop() which is called from src/library/rdt/src/rdtc:Rdt()
    void (*probe_end)();
    // Fires on every R function call.
    // Look for RDT_HOOK(probe_function_entry, call, op, newrho) in src/main/eval.c
    void (*probe_function_entry)(const SEXP call, const SEXP op, const SEXP rho);
    // Fires after every R function call.
    // Look for RDT_HOOK(probe_function_exit, call, op, newrho, tmp) in src/main/eval.c
    void (*probe_function_exit)(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval);
    // Fires on every BUILTINSXP call.
    // Look for RDT_HOOK(probe_builtin_entry ... in src/main/eval.c
    void (*probe_builtin_entry)(const SEXP call, const SEXP op, const SEXP rho);
    // Fires after every BUILTINSXP call.
    // Look for RDT_HOOK(probe_builtin_exit ... in src/main/eval.c
    void (*probe_builtin_exit)(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval);
    // Fires on every SPECIALSXP call.
    // Look for RDT_HOOK(probe_specialsxp_entry ... in src/main/eval.c
    void (*probe_specialsxp_entry)(const SEXP call, const SEXP op, const SEXP rho);
    // Fires after every SPECIALSXP call.
    // Look for RDT_HOOK(probe_specialsxp_exit ... in src/main/eval.c
    void (*probe_specialsxp_exit)(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval);
    // Fires on promise allocation.
    // RDT_HOOK(probe_promise_created, s) in src/main/memory.c:mkPROMISE()
    void (*probe_promise_created)(const SEXP prom, const SEXP rho);
    // Fires when a promise is forced (accessed for the first time)
    // Look for RDT_HOOK(probe_force_promise_entry ... in src/main/eval.c
    void (*probe_force_promise_entry)(const SEXP symbol, const SEXP rho);
    // Fires right after a promise is forced
    // Look for RDT_HOOK(probe_force_promise_exit ... in src/main/eval.c
    void (*probe_force_promise_exit)(const SEXP symbol, const SEXP rho, const SEXP val);
    // Fires when a promise is accessed but already forced
    // Look for RDT_HOOK(probe_promise_lookup ... in src/main/eval.c
    void (*probe_promise_lookup)(const SEXP symbol, const SEXP rho, const SEXP val);
    // RDT_FIRE_PROBE(probe_error, call, message) in src/main/errors.c:errorcall()
    void (*probe_error)(const SEXP call, const char* message);
    // RDT_HOOK(probe_vector_alloc ... in src/main/memory.c:allocVector3()
    void (*probe_vector_alloc)(int sexptype, long length, long bytes, const char* srcref);
    // RDT_HOOK(probe_eval_entry, e, rho) in src/main/eval.c:eval()
    void (*probe_eval_entry)(SEXP e, SEXP rho);
    // RDT_HOOK(probe_eval_exit, e, rho, e) in src/main/eval.c:eval()
    void (*probe_eval_exit)(SEXP e, SEXP rho, SEXP retval);
    // RDT_HOOK(probe_gc_entry, size_needed) in src/main/memory.c:R_gc_internal()
    void (*probe_gc_entry)(R_size_t size_needed);
    // RDT_HOOK(probe_gc_exit, gc_count, vcells, ncells) in src/main/memory.c:R_gc_internal()
    void (*probe_gc_exit)(int gc_count, double vcells, double ncells);
    // Fires when a promise gets garbage collected
    // RDT_HOOK(probe_gc_promise_unmarked, s) in src/main/memory.c:TryToReleasePages()
    void (*probe_gc_promise_unmarked)(const SEXP promise);
    // Fires when the interpreter is about to longjump into a different context.
    // Parameter rho is the target environment.
    // RDT_HOOK(probe_jump_ctxt, env, val) in src/main/context.c:findcontext()
    void (*probe_jump_ctxt)(const SEXP rho, const SEXP val);
    // RDT_HOOK(probe_S3_generic_entry, generic, obj) in src/main/objects.c:usemethod()
    void (*probe_S3_generic_entry)(const char *generic, const SEXP object);
    // RDT_HOOK(probe_S3_generic_exit ... in src/main/objects.c:usemethod()
    void (*probe_S3_generic_exit)(const char *generic, const SEXP object, const SEXP retval);
    // RDT_HOOK(probe_S3_dispatch_entry ... in src/main/objects.c:dispatchMethod()
    void (*probe_S3_dispatch_entry)(const char *generic, const char *clazz, const SEXP method, const SEXP object);
    // RDT_HOOK(probe_S3_dispatch_exit ... in src/main/objects.c:dispatchMethod()
    void (*probe_S3_dispatch_exit)(const char *generic, const char *clazz, const SEXP method, const SEXP object, const SEXP retval);
} rdt_handler;

void rdt_start(const rdt_handler *handler, const SEXP prom);
void rdt_stop();
int rdt_is_running();

// the current handler
extern const rdt_handler *rdt_curr_handler;

// ----------------------------------------------------------------------------
// helpers
// ----------------------------------------------------------------------------

#define CHKSTR(s) ((s) == NULL ? "<unknown>" : (s))

int gc_toggle_off();
void gc_toggle_restore(int previous_value);

const char *get_ns_name(SEXP op);
const char *get_name(SEXP call);
char *get_location(SEXP op);
char *get_callsite(int);
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
