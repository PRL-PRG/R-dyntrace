#ifndef __DYNTRACE_H__
#define __DYNTRACE_H__

#include <R.h>
#include <Rinternals.h>
#include <config.h>
#include <libintl.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#define HAVE_DECL_SIZE_MAX 1
#include <Defn.h>

#ifdef ENABLE_DYNTRACE

#define DYNTRACE_PROBE_HEADER(probe_name)                                      \
  if (dyntrace_active_dyntracer != NULL &&                                     \
      dyntrace_active_dyntracer->probe_name != NULL) {                         \
    CHECK_REENTRANCY(probe_name);                                              \
    dyntrace_active_dyntracer_probe_name = #probe_name;                        \
    dyntrace_disable_garbage_collector();

#define DYNTRACE_PROBE_FOOTER                                                  \
  dyntrace_reinstate_garbage_collector();                                      \
  dyntrace_active_dyntracer_probe_name = NULL;                                 \
  }

#define CHECK_REENTRANCY(probe_name)                                           \
  if (dyntrace_active_dyntracer_probe_name != NULL) {                          \
    printf("[ERROR] - [NESTED HOOK EXECUTION] - %s triggers %s\n",             \
           dyntrace_active_dyntracer_probe_name, #probe_name);                 \
    printf("Exiting program ...\n");                                           \
    exit(1);                                                                   \
  }

#define DYNTRACE_PROBE_BEGIN(prom)                                             \
  DYNTRACE_PROBE_HEADER(probe_begin);                                          \
  PROTECT(prom);                                                               \
  dyntrace_active_dyntracer->probe_begin(dyntrace_active_dyntrace_context,     \
                                         prom);                                \
  UNPROTECT(1);                                                                \
  DYNTRACE_PROBE_FOOTER;

#define DYNTRACE_PROBE_END()                                                   \
  DYNTRACE_PROBE_HEADER(probe_end);                                            \
  dyntrace_active_dyntracer->probe_end(dyntrace_active_dyntrace_context);      \
  DYNTRACE_PROBE_FOOTER;

#define DYNTRACE_SHOULD_PROBE(probe_name)                                      \
  (dyntrace_active_dyntracer->probe_name != NULL)

#define DYNTRACE_PROBE_FUNCTION_ENTRY(call, op, rho)                           \
  DYNTRACE_PROBE_HEADER(probe_function_entry);                                 \
  PROTECT(call);                                                               \
  PROTECT(op);                                                                 \
  PROTECT(rho);                                                                \
  dyntrace_active_dyntracer->probe_function_entry(                             \
      dyntrace_active_dyntrace_context, call, op, rho);                        \
  UNPROTECT(3);                                                                \
  DYNTRACE_PROBE_FOOTER;

#define DYNTRACE_PROBE_FUNCTION_EXIT(call, op, rho, retval)                    \
  DYNTRACE_PROBE_HEADER(probe_function_exit);                                  \
  PROTECT(call);                                                               \
  PROTECT(op);                                                                 \
  PROTECT(rho);                                                                \
  PROTECT(retval);                                                             \
  dyntrace_active_dyntracer->probe_function_exit(                              \
      dyntrace_active_dyntrace_context, call, op, rho, retval);                \
  UNPROTECT(4);                                                                \
  DYNTRACE_PROBE_FOOTER;

#define DYNTRACE_PROBE_BUILTIN_ENTRY(call, op, rho)                            \
  DYNTRACE_PROBE_HEADER(probe_builtin_entry);                                  \
  PROTECT(call);                                                               \
  PROTECT(op);                                                                 \
  PROTECT(rho);                                                                \
  dyntrace_active_dyntracer->probe_builtin_entry(                              \
      dyntrace_active_dyntrace_context, call, op, rho);                        \
  UNPROTECT(3);                                                                \
  DYNTRACE_PROBE_FOOTER;

#define DYNTRACE_PROBE_BUILTIN_EXIT(call, op, rho, retval)                     \
  DYNTRACE_PROBE_HEADER(probe_builtin_exit);                                   \
  PROTECT(call);                                                               \
  PROTECT(op);                                                                 \
  PROTECT(rho);                                                                \
  PROTECT(retval);                                                             \
  dyntrace_active_dyntracer->probe_builtin_exit(                               \
      dyntrace_active_dyntrace_context, call, op, rho, retval);                \
  UNPROTECT(4);                                                                \
  DYNTRACE_PROBE_FOOTER;

#define DYNTRACE_PROBE_SPECIALSXP_ENTRY(call, op, rho)                         \
  DYNTRACE_PROBE_HEADER(probe_specialsxp_entry);                               \
  PROTECT(call);                                                               \
  PROTECT(op);                                                                 \
  PROTECT(rho);                                                                \
  dyntrace_active_dyntracer->probe_specialsxp_entry(                           \
      dyntrace_active_dyntrace_context, call, op, rho);                        \
  UNPROTECT(3);                                                                \
  DYNTRACE_PROBE_FOOTER;

#define DYNTRACE_PROBE_SPECIALSXP_EXIT(call, op, rho, retval)                  \
  DYNTRACE_PROBE_HEADER(probe_specialsxp_exit);                                \
  PROTECT(call);                                                               \
  PROTECT(op);                                                                 \
  PROTECT(rho);                                                                \
  PROTECT(retval);                                                             \
  dyntrace_active_dyntracer->probe_specialsxp_exit(                            \
      dyntrace_active_dyntrace_context, call, op, rho, retval);                \
  UNPROTECT(4);                                                                \
  DYNTRACE_PROBE_FOOTER;

#define DYNTRACE_PROBE_PROMISE_CREATED(prom, rho)                              \
  DYNTRACE_PROBE_HEADER(probe_promise_created);                                \
  PROTECT(prom);                                                               \
  PROTECT(rho);                                                                \
  dyntrace_active_dyntracer->probe_promise_created(                            \
      dyntrace_active_dyntrace_context, prom, rho);                            \
  UNPROTECT(2);                                                                \
  DYNTRACE_PROBE_FOOTER;

#define DYNTRACE_PROBE_PROMISE_FORCE_ENTRY(symbol, rho)                        \
  DYNTRACE_PROBE_HEADER(probe_promise_force_entry);                            \
  PROTECT(symbol);                                                             \
  PROTECT(rho);                                                                \
  dyntrace_active_dyntracer->probe_promise_force_entry(                        \
      dyntrace_active_dyntrace_context, symbol, rho);                          \
  UNPROTECT(2);                                                                \
  DYNTRACE_PROBE_FOOTER;

#define DYNTRACE_PROBE_PROMISE_FORCE_EXIT(symbol, rho, val)                    \
  DYNTRACE_PROBE_HEADER(probe_promise_force_exit);                             \
  PROTECT(symbol);                                                             \
  PROTECT(rho);                                                                \
  PROTECT(val);                                                                \
  dyntrace_active_dyntracer->probe_promise_force_exit(                         \
      dyntrace_active_dyntrace_context, symbol, rho, val);                     \
  UNPROTECT(3);                                                                \
  DYNTRACE_PROBE_FOOTER;

#define DYNTRACE_PROBE_PROMISE_VALUE_LOOKUP(symbol, rho, val)                  \
  DYNTRACE_PROBE_HEADER(probe_promise_value_lookup);                           \
  PROTECT(symbol);                                                             \
  PROTECT(rho);                                                                \
  PROTECT(val);                                                                \
  dyntrace_active_dyntracer->probe_promise_value_lookup(                       \
      dyntrace_active_dyntrace_context, symbol, rho, val);                     \
  UNPROTECT(3);                                                                \
  DYNTRACE_PROBE_FOOTER;

#define DYNTRACE_PROBE_PROMISE_EXPRESSION_LOOKUP(prom, rho)                    \
  DYNTRACE_PROBE_HEADER(probe_promise_expression_lookup);                      \
  PROTECT(prom);                                                               \
  PROTECT(rho);                                                                \
  dyntrace_active_dyntracer->probe_promise_expression_lookup(                  \
      dyntrace_active_dyntrace_context, prom, rho);                            \
  UNPROTECT(2);                                                                \
  DYNTRACE_PROBE_FOOTER;

#define DYNTRACE_PROBE_ERROR(call, message)                                    \
  DYNTRACE_PROBE_HEADER(probe_error);                                          \
  PROTECT(call);                                                               \
  dyntrace_active_dyntracer->probe_error(dyntrace_active_dyntrace_context,     \
                                         call, message);                       \
  UNPROTECT(1);                                                                \
  DYNTRACE_PROBE_FOOTER;

#define DYNTRACE_PROBE_VECTOR_ALLOC(sexptype, length, bytes, srcref)           \
  DYNTRACE_PROBE_HEADER(probe_vector_alloc);                                   \
  dyntrace_active_dyntracer->probe_vector_alloc(                               \
      dyntrace_active_dyntrace_context, sexptype, length, bytes, srcref);      \
  DYNTRACE_PROBE_FOOTER;

#define DYNTRACE_PROBE_EVAL_ENTRY(e, rho)                                      \
  DYNTRACE_PROBE_HEADER(probe_eval_entry);                                     \
  PROTECT(e);                                                                  \
  PROTECT(rho);                                                                \
  dyntrace_active_dyntracer->probe_eval_entry(                                 \
      dyntrace_active_dyntrace_context, e, rho);                               \
  UNPROTECT(2);                                                                \
  DYNTRACE_PROBE_FOOTER;

#define DYNTRACE_PROBE_EVAL_EXIT(e, rho, retval)                               \
  DYNTRACE_PROBE_HEADER(probe_eval_exit);                                      \
  PROTECT(e);                                                                  \
  PROTECT(rho);                                                                \
  PROTECT(retval);                                                             \
  dyntrace_active_dyntracer->probe_eval_exit(dyntrace_active_dyntrace_context, \
                                             e, rho, retval);                  \
  UNPROTECT(3);                                                                \
  DYNTRACE_PROBE_FOOTER;

#define DYNTRACE_PROBE_GC_ENTRY(size_needed)                                   \
  DYNTRACE_PROBE_HEADER(probe_gc_entry);                                       \
  dyntrace_active_dyntracer->probe_gc_entry(dyntrace_active_dyntrace_context,  \
                                            size_needed);                      \
  DYNTRACE_PROBE_FOOTER;

#define DYNTRACE_PROBE_GC_EXIT(gc_count, vcells, ncells)                       \
  DYNTRACE_PROBE_HEADER(probe_gc_exit);                                        \
  dyntrace_active_dyntracer->probe_gc_exit(dyntrace_active_dyntrace_context,   \
                                           gc_count, vcells, ncells);          \
  DYNTRACE_PROBE_FOOTER;

#define DYNTRACE_PROBE_GC_PROMISE_UNMARKED(promise)                            \
  DYNTRACE_PROBE_HEADER(probe_gc_promise_unmarked);                            \
  PROTECT(promise);                                                            \
  dyntrace_active_dyntracer->probe_gc_promise_unmarked(                        \
      dyntrace_active_dyntrace_context, promise);                              \
  UNPROTECT(1);                                                                \
  DYNTRACE_PROBE_FOOTER;

#define DYNTRACE_PROBE_JUMP_CTXT(rho, val)                                     \
  DYNTRACE_PROBE_HEADER(probe_jump_ctxt);                                      \
  PROTECT(rho);                                                                \
  PROTECT(val);                                                                \
  dyntrace_active_dyntracer->probe_jump_ctxt(dyntrace_active_dyntrace_context, \
                                             rho, val);                        \
  UNPROTECT(2);                                                                \
  DYNTRACE_PROBE_FOOTER;

#define DYNTRACE_PROBE_S3_GENERIC_ENTRY(generic, object)                       \
  DYNTRACE_PROBE_HEADER(probe_S3_generic_entry);                               \
  PROTECT(object);                                                             \
  dyntrace_active_dyntracer->probe_S3_generic_entry(                           \
      dyntrace_active_dyntrace_context, generic, object);                      \
  UNPROTECT(1);                                                                \
  DYNTRACE_PROBE_FOOTER;

#define DYNTRACE_PROBE_S3_GENERIC_EXIT(generic, object, retval)                \
  DYNTRACE_PROBE_HEADER(probe_S3_generic_exit);                                \
  PROTECT(object);                                                             \
  PROTECT(retval);                                                             \
  dyntrace_active_dyntracer->probe_S3_generic_exit(                            \
      dyntrace_active_dyntrace_context, generic, object, retval);              \
  UNPROTECT(2);                                                                \
  DYNTRACE_PROBE_FOOTER;

#define DYNTRACE_PROBE_S3_DISPATCH_ENTRY(generic, clazz, method, object)       \
  DYNTRACE_PROBE_HEADER(probe_S3_dispatch_entry);                              \
  PROTECT(method);                                                             \
  PROTECT(object);                                                             \
  dyntrace_active_dyntracer->probe_S3_dispatch_entry(                          \
      dyntrace_active_dyntrace_context, generic, clazz, method, object);       \
  UNPROTECT(2);                                                                \
  DYNTRACE_PROBE_FOOTER;

#define DYNTRACE_PROBE_S3_DISPATCH_EXIT(generic, clazz, method, object,        \
                                        retval)                                \
  DYNTRACE_PROBE_HEADER(probe_S3_dispatch_exit);                               \
  PROTECT(method);                                                             \
  PROTECT(object);                                                             \
  PROTECT(retval);                                                             \
  dyntrace_active_dyntracer->probe_S3_dispatch_exit(                           \
      dyntrace_active_dyntrace_context, generic, clazz, method, object,        \
      retval);                                                                 \
  UNPROTECT(3);                                                                \
  DYNTRACE_PROBE_FOOTER;

#else
#define DYNTRACE_PROBE_BEGIN(prom)
#define DYNTRACE_PROBE_END()
#define DYNTRACE_PROBE_FUNCTION_ENTRY(call, op, rho)
#define DYNTRACE_PROBE_FUNCTION_EXIT(call, op, rho, retval)
#define DYNTRACE_PROBE_BUILTIN_ENTRY(call, op, rho)
#define DYNTRACE_PROBE_BUILTIN_EXIT(call, op, rho, retval)
#define DYNTRACE_PROBE_SPECIALSXP_ENTRY(call, op, rho)
#define DYNTRACE_PROBE_SPECIALSXP_EXIT(call, op, rho, retval)
#define DYNTRACE_PROBE_PROMISE_CREATED(prom, rho)
#define DYNTRACE_PROBE_PROMISE_FORCE_ENTRY(symbol, rho)
#define DYNTRACE_PROBE_PROMISE_FORCE_EXIT(symbol, rho, val)
#define DYNTRACE_PROBE_PROMISE_VALUE_LOOKUP(symbol, rho, val)
#define DYNTRACE_PROBE_PROMISE_EXPRESSION_LOOKUP(prom, rho)
#define DYNTRACE_PROBE_ERROR(call, message)
#define DYNTRACE_PROBE_VECTOR_ALLOC(sexptype, length, bytes, scref)
#define DYNTRACE_PROBE_EVAL_ENTRY(e, rho)
#define DYNTRACE_PROBE_EVAL_EXIT(e, rho, retval)
#define DYNTRACE_PROBE_GC_ENTRY(size_needed)
#define DYNTRACE_PROBE_GC_EXIT(gc_count, vcells, ncells)
#define DYNTRACE_PROBE_GC_PROMISE_UNMARKED(promise)
#define DYNTRACE_PROBE_JUMP_CTXT(rho, val)
#define DYNTRACE_PROBE_S3_GENERIC_ENTRY(generic, object)
#define DYNTRACE_PROBE_S3_GENERIC_EXIT(generic, object, retval)
#define DYNTRACE_PROBE_S3_DISPATCH_ENTRY(generic, clazz, method, object)
#define DYNTRACE_PROBE_S3_DISPATCH_EXIT(generic, clazz, method, object, retval)

#endif

/* ----------------------------------------------------------------------------
DYNTRACE TYPE DEFINITIONS
---------------------------------------------------------------------------- */
typedef struct {
  int begin_time;
  int end_time;
} dyntracing_context_t;

typedef struct {
  void *dyntracer_context;
  dyntracing_context_t *dyntracing_context;
} dyntrace_context_t;

typedef struct {
  // Fires when the tracer starts.
  // Not an interpreter hook. It is called from
  // src/main/rdtrace.c:rdt_start() which is called from
  // src/library/rdt/src/rdt.c:Rdt() (the entrypoint)
  void (*probe_begin)(dyntrace_context_t *dyntrace_context, const SEXP prom);
  // Fires when the tracer ends (after the traced call returns)
  // Not an interpreter hook. It is called from
  // src/main/rdtrace.c:rdt_stop() which is called from
  // src/library/rdt/src/rdtc:Rdt()
  void (*probe_end)(dyntrace_context_t *dyntrace_context);
  // Fires on every R function call.
  // Look for RDT_HOOK(probe_function_entry, call, op, newrho) in
  // src/main/eval.c
  void (*probe_function_entry)(dyntrace_context_t *dyntrace_context,
                               const SEXP call, const SEXP op, const SEXP rho);
  // Fires after every R function call.
  // Look for RDT_HOOK(probe_function_exit, call, op, newrho, tmp) in
  // src/main/eval.c
  void (*probe_function_exit)(dyntrace_context_t *dyntrace_context,
                              const SEXP call, const SEXP op, const SEXP rho,
                              const SEXP retval);
  // Fires on every BUILTINSXP call.
  // Look for RDT_HOOK(probe_builtin_entry ... in src/main/eval.c
  void (*probe_builtin_entry)(dyntrace_context_t *dyntrace_context,
                              const SEXP call, const SEXP op, const SEXP rho);
  // Fires after every BUILTINSXP call.
  // Look for RDT_HOOK(probe_builtin_exit ... in src/main/eval.c
  void (*probe_builtin_exit)(dyntrace_context_t *dyntrace_context,
                             const SEXP call, const SEXP op, const SEXP rho,
                             const SEXP retval);
  // Fires on every SPECIALSXP call.
  // Look for RDT_HOOK(probe_specialsxp_entry ... in src/main/eval.c
  void (*probe_specialsxp_entry)(dyntrace_context_t *dyntrace_context,
                                 const SEXP call, const SEXP op,
                                 const SEXP rho);
  // Fires after every SPECIALSXP call.
  // Look for RDT_HOOK(probe_specialsxp_exit ... in src/main/eval.c
  void (*probe_specialsxp_exit)(dyntrace_context_t *dyntrace_context,
                                const SEXP call, const SEXP op, const SEXP rho,
                                const SEXP retval);
  // Fires on promise allocation.
  // DYNTRACE_PROBE_PROMISE_CREATED(...) in src/main/memory.c:mkPROMISE()
  void (*probe_promise_created)(dyntrace_context_t *dyntrace_context,
                                const SEXP prom, const SEXP rho);
  // Fires when a promise is forced (accessed for the first time)
  // Look for DYNTRACE_PROBE_PROMISE_FORCE_ENTRY(...) in src/main/eval.c
  void (*probe_promise_force_entry)(dyntrace_context_t *dyntrace_context,
                                    const SEXP symbol, const SEXP rho);
  // Fires right after a promise is forced
  // Look for DYNTRACE_PROBE_PROMISE_FORCE_EXIT(...) in src/main/eval.c
  void (*probe_promise_force_exit)(dyntrace_context_t *dyntrace_context,
                                   const SEXP symbol, const SEXP rho,
                                   const SEXP val);
  // Fires when a promise is accessed but already forced
  // Look for DYNTRACE_PROBE_PROMISE_VALUE_LOOKUP(...) in src/main/eval.c
  void (*probe_promise_value_lookup)(dyntrace_context_t *dyntrace_context,
                                     const SEXP symbol, const SEXP rho,
                                     const SEXP val);
  // Fires when the expression inside a promise is accessed
  // Look for DYNTRACE_PROBE_PROMISE_EXPRESSION_LOOKUP(...) in
  // src/main/coerce.c
  void (*probe_promise_expression_lookup)(dyntrace_context_t *dyntrace_context,
                                          const SEXP prom, const SEXP rho);
  // DYNTRACE_PROBE_ERROR(call, message) in
  // src/main/errors.c:errorcall()
  void (*probe_error)(dyntrace_context_t *dyntrace_context, const SEXP call,
                      const char *message);
  // RDT_HOOK(probe_vector_alloc ... in src/main/memory.c:allocVector3()
  void (*probe_vector_alloc)(dyntrace_context_t *dyntrace_context, int sexptype,
                             long length, long bytes, const char *srcref);
  // RDT_HOOK(probe_eval_entry, e, rho) in src/main/eval.c:eval()
  void (*probe_eval_entry)(dyntrace_context_t *dyntrace_context, SEXP e,
                           SEXP rho);
  // RDT_HOOK(probe_eval_exit, e, rho, e) in src/main/eval.c:eval()
  void (*probe_eval_exit)(dyntrace_context_t *dyntrace_context, SEXP e,
                          SEXP rho, SEXP retval);
  // RDT_HOOK(probe_gc_entry, size_needed) in
  // src/main/memory.c:R_gc_internal()
  void (*probe_gc_entry)(dyntrace_context_t *dyntrace_context,
                         R_size_t size_needed);
  // RDT_HOOK(probe_gc_exit, gc_count, vcells, ncells) in
  // src/main/memory.c:R_gc_internal()
  void (*probe_gc_exit)(dyntrace_context_t *dyntrace_context, int gc_count,
                        double vcells, double ncells);
  // Fires when a promise gets garbage collected
  // RDT_HOOK(probe_gc_promise_unmarked, s) in
  // src/main/memory.c:TryToReleasePages()
  void (*probe_gc_promise_unmarked)(dyntrace_context_t *dyntrace_context,
                                    const SEXP promise);
  // Fires when the interpreter is about to longjump into a different context.
  // Parameter rho is the target environment.
  // RDT_HOOK(probe_jump_ctxt, env, val) in src/main/context.c:findcontext()
  void (*probe_jump_ctxt)(dyntrace_context_t *dyntrace_context, const SEXP rho,
                          const SEXP val);

  // RDT_HOOK(probe_S3_generic_entry, generic, obj) in
  // src/main/objects.c:usemethod()
  void (*probe_S3_generic_entry)(dyntrace_context_t *dyntrace_context,
                                 const char *generic, const SEXP object);
  // RDT_HOOK(probe_S3_generic_exit ... in src/main/objects.c:usemethod()
  void (*probe_S3_generic_exit)(dyntrace_context_t *dyntrace_context,
                                const char *generic, const SEXP object,
                                const SEXP retval);
  // RDT_HOOK(probe_S3_dispatch_entry ... in
  // src/main/objects.c:dispatchMethod()
  void (*probe_S3_dispatch_entry)(dyntrace_context_t *dyntrace_context,
                                  const char *generic, const char *clazz,
                                  const SEXP method, const SEXP object);
  // RDT_HOOK(probe_S3_dispatch_exit ... in
  // src/main/objects.c:dispatchMethod()
  void (*probe_S3_dispatch_exit)(dyntrace_context_t *dyntrace_context,
                                 const char *generic, const char *clazz,
                                 const SEXP method, const SEXP object,
                                 const SEXP retval);
  void *context;
} dyntracer_t;

// ----------------------------------------------------------------------------
// STATE VARIABLES - For Internal Use Only
// ----------------------------------------------------------------------------

// the current dyntracer
extern dyntracer_t *dyntrace_active_dyntracer;
// name of currently executing probe
extern const char *dyntrace_active_dyntracer_probe_name;
// state of garbage collector before the hook is triggered
extern int dyntrace_garbage_collector_state;
// context of dyntrace
extern dyntrace_context_t *dyntrace_active_dyntrace_context;

SEXP do_dyntrace(SEXP call, SEXP op, SEXP args, SEXP rho);
int dyntrace_is_active();
int dyntrace_should_probe();
int dyntrace_dyntracer_is_active();
int dyntrace_dyntracer_probe_is_active();
void dyntrace_disable_garbage_collector();
void dyntrace_reinstate_garbage_collector();
dyntracer_t *dyntrace_unwrap_dyntracer(SEXP wrapped_dyntracer);
SEXP dyntrace_wrap_dyntracer(dyntracer_t *dyntracer, const char *classname);

// ----------------------------------------------------------------------------
// helpers
// ----------------------------------------------------------------------------

#define CHKSTR(s) ((s) == NULL ? "<unknown>" : (s))
SEXP get_named_list_element(const SEXP list, const char *name);
const char *serialize_sexp(SEXP s);
const char *get_string(SEXP sexp);
#ifdef __cplusplus
}
#endif

#define RDT_HOOK(name, ...)                                                    \
  if (RDT_IS_ENABLED(name)) {                                                  \
    if (tracing_is_active()) {                                                 \
      printf("[ERROR] - [NESTED HOOK EXECUTION] - %s triggers %s\n",           \
             RDT_ACTIVE_HOOK_NAME, #name);                                     \
      printf("Exiting program ...\n");                                         \
      exit(1);                                                                 \
    }                                                                          \
    RDT_ACTIVE_HOOK_NAME = #name;                                              \
    disable_garbage_collector();                                               \
    RDT_FIRE_PROBE(name, (rdt_curr_handler->context), ##__VA_ARGS__);          \
    reinstate_garbage_collector();                                             \
    RDT_ACTIVE_HOOK_NAME = NULL;                                               \
  }

#endif /* __DYNTRACE_H__ */
