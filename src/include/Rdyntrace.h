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
      dyntrace_active_dyntracer->probe_name != NULL &&                         \
      !dyntrace_is_priviliged_mode()) {                                        \
    dyntrace_active_dyntrace_context->dyntracing_context->execution_time       \
        .expression += dyntrace_reset_stopwatch();                             \
    dyntrace_active_dyntrace_context->dyntracing_context->execution_count      \
        .probe_name++;                                                         \
    CHECK_REENTRANCY(probe_name);                                              \
    dyntrace_active_dyntracer_probe_name = #probe_name;                        \
    dyntrace_disable_garbage_collector();

#define DYNTRACE_PROBE_FOOTER(probe_name)                                      \
  dyntrace_reinstate_garbage_collector();                                      \
  dyntrace_active_dyntracer_probe_name = NULL;                                 \
  dyntrace_active_dyntrace_context->dyntracing_context->execution_time         \
      .probe_name += dyntrace_reset_stopwatch();                               \
  }

#define CHECK_REENTRANCY(probe_name)                                           \
  if (dyntrace_active_dyntracer_probe_name != NULL) {                          \
    Rf_error("[ERROR] - [NESTED HOOK EXECUTION] - %s triggers %s\n",           \
             dyntrace_active_dyntracer_probe_name, #probe_name);               \
  }

#define DYNTRACE_PROBE_BEGIN(prom)                                             \
  DYNTRACE_PROBE_HEADER(probe_begin);                                          \
  PROTECT(prom);                                                               \
  dyntrace_active_dyntracer->probe_begin(dyntrace_active_dyntrace_context,     \
                                         prom);                                \
  UNPROTECT(1);                                                                \
  DYNTRACE_PROBE_FOOTER(probe_begin);

#define DYNTRACE_PROBE_END()                                                   \
  DYNTRACE_PROBE_HEADER(probe_end);                                            \
  dyntrace_active_dyntracer->probe_end(dyntrace_active_dyntrace_context);      \
  DYNTRACE_PROBE_FOOTER(probe_end);

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
  DYNTRACE_PROBE_FOOTER(probe_function_entry);

#define DYNTRACE_PROBE_FUNCTION_EXIT(call, op, rho, retval)                    \
  DYNTRACE_PROBE_HEADER(probe_function_exit);                                  \
  PROTECT(call);                                                               \
  PROTECT(op);                                                                 \
  PROTECT(rho);                                                                \
  PROTECT(retval);                                                             \
  dyntrace_active_dyntracer->probe_function_exit(                              \
      dyntrace_active_dyntrace_context, call, op, rho, retval);                \
  UNPROTECT(4);                                                                \
  DYNTRACE_PROBE_FOOTER(probe_function_exit);

#define DYNTRACE_PROBE_BUILTIN_ENTRY(call, op, rho)                            \
  DYNTRACE_PROBE_HEADER(probe_builtin_entry);                                  \
  PROTECT(call);                                                               \
  PROTECT(op);                                                                 \
  PROTECT(rho);                                                                \
  dyntrace_active_dyntracer->probe_builtin_entry(                              \
      dyntrace_active_dyntrace_context, call, op, rho);                        \
  UNPROTECT(3);                                                                \
  DYNTRACE_PROBE_FOOTER(probe_builtin_entry);

#define DYNTRACE_PROBE_BUILTIN_EXIT(call, op, rho, retval)                     \
  DYNTRACE_PROBE_HEADER(probe_builtin_exit);                                   \
  PROTECT(call);                                                               \
  PROTECT(op);                                                                 \
  PROTECT(rho);                                                                \
  PROTECT(retval);                                                             \
  dyntrace_active_dyntracer->probe_builtin_exit(                               \
      dyntrace_active_dyntrace_context, call, op, rho, retval);                \
  UNPROTECT(4);                                                                \
  DYNTRACE_PROBE_FOOTER(probe_builtin_exit);

#define DYNTRACE_PROBE_SPECIALSXP_ENTRY(call, op, rho)                         \
  DYNTRACE_PROBE_HEADER(probe_specialsxp_entry);                               \
  PROTECT(call);                                                               \
  PROTECT(op);                                                                 \
  PROTECT(rho);                                                                \
  dyntrace_active_dyntracer->probe_specialsxp_entry(                           \
      dyntrace_active_dyntrace_context, call, op, rho);                        \
  UNPROTECT(3);                                                                \
  DYNTRACE_PROBE_FOOTER(probe_specialsxp_entry);

#define DYNTRACE_PROBE_SPECIALSXP_EXIT(call, op, rho, retval)                  \
  DYNTRACE_PROBE_HEADER(probe_specialsxp_exit);                                \
  PROTECT(call);                                                               \
  PROTECT(op);                                                                 \
  PROTECT(rho);                                                                \
  PROTECT(retval);                                                             \
  dyntrace_active_dyntracer->probe_specialsxp_exit(                            \
      dyntrace_active_dyntrace_context, call, op, rho, retval);                \
  UNPROTECT(4);                                                                \
  DYNTRACE_PROBE_FOOTER(probe_specialsxp_exit);

#define DYNTRACE_PROBE_PROMISE_CREATED(prom, rho)                              \
  DYNTRACE_PROBE_HEADER(probe_promise_created);                                \
  PROTECT(prom);                                                               \
  PROTECT(rho);                                                                \
  dyntrace_active_dyntracer->probe_promise_created(                            \
      dyntrace_active_dyntrace_context, prom, rho);                            \
  UNPROTECT(2);                                                                \
  DYNTRACE_PROBE_FOOTER(probe_promise_created);

#define DYNTRACE_PROBE_PROMISE_FORCE_ENTRY(promise)                            \
  DYNTRACE_PROBE_HEADER(probe_promise_force_entry);                            \
  PROTECT(promise);                                                            \
  dyntrace_active_dyntracer->probe_promise_force_entry(                        \
      dyntrace_active_dyntrace_context, promise);                              \
  UNPROTECT(1);                                                                \
  DYNTRACE_PROBE_FOOTER(probe_promise_force_entry);

#define DYNTRACE_PROBE_PROMISE_FORCE_EXIT(promise)                             \
  DYNTRACE_PROBE_HEADER(probe_promise_force_exit);                             \
  PROTECT(promise);                                                            \
  dyntrace_active_dyntracer->probe_promise_force_exit(                         \
      dyntrace_active_dyntrace_context, promise);                              \
  UNPROTECT(1);                                                                \
  DYNTRACE_PROBE_FOOTER(probe_promise_force_exit);

#define DYNTRACE_PROBE_PROMISE_VALUE_LOOKUP(promise)                           \
  DYNTRACE_PROBE_HEADER(probe_promise_value_lookup);                           \
  PROTECT(promise);                                                            \
  dyntrace_active_dyntracer->probe_promise_value_lookup(                       \
      dyntrace_active_dyntrace_context, promise);                              \
  UNPROTECT(1);                                                                \
  DYNTRACE_PROBE_FOOTER(probe_promise_value_lookup);

#define DYNTRACE_PROBE_PROMISE_EXPRESSION_LOOKUP(promise)                      \
  DYNTRACE_PROBE_HEADER(probe_promise_expression_lookup);                      \
  PROTECT(promise);                                                            \
  dyntrace_active_dyntracer->probe_promise_expression_lookup(                  \
      dyntrace_active_dyntrace_context, promise);                              \
  UNPROTECT(1);                                                                \
  DYNTRACE_PROBE_FOOTER(probe_promise_expression_lookup);

#define DYNTRACE_PROBE_ERROR(call, message)                                    \
  DYNTRACE_PROBE_HEADER(probe_error);                                          \
  PROTECT(call);                                                               \
  dyntrace_active_dyntracer->probe_error(dyntrace_active_dyntrace_context,     \
                                         call, message);                       \
  UNPROTECT(1);                                                                \
  DYNTRACE_PROBE_FOOTER(probe_error);

#define DYNTRACE_PROBE_VECTOR_ALLOC(sexptype, length, bytes, srcref)           \
  DYNTRACE_PROBE_HEADER(probe_vector_alloc);                                   \
  dyntrace_active_dyntracer->probe_vector_alloc(                               \
      dyntrace_active_dyntrace_context, sexptype, length, bytes, srcref);      \
  DYNTRACE_PROBE_FOOTER(probe_vector_alloc);

#define DYNTRACE_PROBE_EVAL_ENTRY(e, rho)                                      \
  DYNTRACE_PROBE_HEADER(probe_eval_entry);                                     \
  PROTECT(e);                                                                  \
  PROTECT(rho);                                                                \
  dyntrace_active_dyntracer->probe_eval_entry(                                 \
      dyntrace_active_dyntrace_context, e, rho);                               \
  UNPROTECT(2);                                                                \
  DYNTRACE_PROBE_FOOTER(probe_eval_entry);

#define DYNTRACE_PROBE_EVAL_EXIT(e, rho, retval)                               \
  DYNTRACE_PROBE_HEADER(probe_eval_exit);                                      \
  PROTECT(e);                                                                  \
  PROTECT(rho);                                                                \
  PROTECT(retval);                                                             \
  dyntrace_active_dyntracer->probe_eval_exit(dyntrace_active_dyntrace_context, \
                                             e, rho, retval);                  \
  UNPROTECT(3);                                                                \
  DYNTRACE_PROBE_FOOTER(probe_eval_exit);

#define DYNTRACE_PROBE_GC_ENTRY(size_needed)                                   \
  DYNTRACE_PROBE_HEADER(probe_gc_entry);                                       \
  dyntrace_active_dyntracer->probe_gc_entry(dyntrace_active_dyntrace_context,  \
                                            size_needed);                      \
  DYNTRACE_PROBE_FOOTER(probe_gc_entry);

#define DYNTRACE_PROBE_GC_EXIT(gc_count, vcells, ncells)                       \
  DYNTRACE_PROBE_HEADER(probe_gc_exit);                                        \
  dyntrace_active_dyntracer->probe_gc_exit(dyntrace_active_dyntrace_context,   \
                                           gc_count, vcells, ncells);          \
  DYNTRACE_PROBE_FOOTER(probe_gc_exit);

#define DYNTRACE_PROBE_GC_PROMISE_UNMARKED(promise)                            \
  DYNTRACE_PROBE_HEADER(probe_gc_promise_unmarked);                            \
  PROTECT(promise);                                                            \
  dyntrace_active_dyntracer->probe_gc_promise_unmarked(                        \
      dyntrace_active_dyntrace_context, promise);                              \
  UNPROTECT(1);                                                                \
  DYNTRACE_PROBE_FOOTER(probe_gc_promise_unmarked);

#define DYNTRACE_PROBE_JUMP_CTXT(rho, val)                                     \
  DYNTRACE_PROBE_HEADER(probe_jump_ctxt);                                      \
  PROTECT(rho);                                                                \
  PROTECT(val);                                                                \
  dyntrace_active_dyntracer->probe_jump_ctxt(dyntrace_active_dyntrace_context, \
                                             rho, val);                        \
  UNPROTECT(2);                                                                \
  DYNTRACE_PROBE_FOOTER(probe_jump_ctxt);

#define DYNTRACE_PROBE_NEW_ENVIRONMENT(rho)                                    \
  DYNTRACE_PROBE_HEADER(probe_new_environment);                                \
  PROTECT(rho);                                                                \
  dyntrace_active_dyntracer->probe_new_environment(                            \
      dyntrace_active_dyntrace_context, rho);                                  \
  UNPROTECT(1);                                                                \
  DYNTRACE_PROBE_FOOTER(probe_new_environment);

#define DYNTRACE_PROBE_S3_GENERIC_ENTRY(generic, object)                       \
  DYNTRACE_PROBE_HEADER(probe_S3_generic_entry);                               \
  PROTECT(object);                                                             \
  dyntrace_active_dyntracer->probe_S3_generic_entry(                           \
      dyntrace_active_dyntrace_context, generic, object);                      \
  UNPROTECT(1);                                                                \
  DYNTRACE_PROBE_FOOTER(probe_S3_generic_entry);

#define DYNTRACE_PROBE_S3_GENERIC_EXIT(generic, object, retval)                \
  DYNTRACE_PROBE_HEADER(probe_S3_generic_exit);                                \
  PROTECT(object);                                                             \
  PROTECT(retval);                                                             \
  dyntrace_active_dyntracer->probe_S3_generic_exit(                            \
      dyntrace_active_dyntrace_context, generic, object, retval);              \
  UNPROTECT(2);                                                                \
  DYNTRACE_PROBE_FOOTER(probe_S3_generic_exit);

#define DYNTRACE_PROBE_S3_DISPATCH_ENTRY(generic, clazz, method, object)       \
  DYNTRACE_PROBE_HEADER(probe_S3_dispatch_entry);                              \
  PROTECT(method);                                                             \
  PROTECT(object);                                                             \
  dyntrace_active_dyntracer->probe_S3_dispatch_entry(                          \
      dyntrace_active_dyntrace_context, generic, clazz, method, object);       \
  UNPROTECT(2);                                                                \
  DYNTRACE_PROBE_FOOTER(probe_S3_dispatch_entry);

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
  DYNTRACE_PROBE_FOOTER(probe_S3_dispatch_exit);

#define DYNTRACE_PROBE_ENVIRONMENT_DEFINE_VAR(symbol, value, rho)              \
  DYNTRACE_PROBE_HEADER(probe_environment_define_var);                         \
  PROTECT(symbol);                                                             \
  PROTECT(value);                                                              \
  PROTECT(rho);                                                                \
  dyntrace_active_dyntracer->probe_environment_define_var(                     \
      dyntrace_active_dyntrace_context, symbol, value, rho);                   \
  UNPROTECT(3);                                                                \
  DYNTRACE_PROBE_FOOTER(probe_environment_define_var);

#define DYNTRACE_PROBE_ENVIRONMENT_ASSIGN_VAR(symbol, value, rho)              \
  DYNTRACE_PROBE_HEADER(probe_environment_assign_var);                         \
  PROTECT(symbol);                                                             \
  PROTECT(value);                                                              \
  PROTECT(rho);                                                                \
  dyntrace_active_dyntracer->probe_environment_assign_var(                     \
      dyntrace_active_dyntrace_context, symbol, value, rho);                   \
  UNPROTECT(3);                                                                \
  DYNTRACE_PROBE_FOOTER(probe_environment_assign_var);

#define DYNTRACE_PROBE_ENVIRONMENT_REMOVE_VAR(symbol, rho)                     \
  DYNTRACE_PROBE_HEADER(probe_environment_remove_var);                         \
  PROTECT(symbol);                                                             \
  PROTECT(rho);                                                                \
  dyntrace_active_dyntracer->probe_environment_remove_var(                     \
      dyntrace_active_dyntrace_context, symbol, rho);                          \
  UNPROTECT(2);                                                                \
  DYNTRACE_PROBE_FOOTER(probe_environment_remove_var);

#define DYNTRACE_PROBE_ENVIRONMENT_LOOKUP_VAR(symbol, value, rho)              \
  DYNTRACE_PROBE_HEADER(probe_environment_lookup_var);                         \
  PROTECT(symbol);                                                             \
  PROTECT(value);                                                              \
  PROTECT(rho);                                                                \
  dyntrace_active_dyntracer->probe_environment_lookup_var(                     \
      dyntrace_active_dyntrace_context, symbol, value, rho);                   \
  UNPROTECT(3);                                                                \
  DYNTRACE_PROBE_FOOTER(probe_environment_lookup_var);

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
#define DYNTRACE_PROBE_PROMISE_FORCE_ENTRY(promise)
#define DYNTRACE_PROBE_PROMISE_FORCE_EXIT(promise)
#define DYNTRACE_PROBE_PROMISE_VALUE_LOOKUP(promise)
#define DYNTRACE_PROBE_PROMISE_EXPRESSION_LOOKUP(promise)
#define DYNTRACE_PROBE_ERROR(call, message)
#define DYNTRACE_PROBE_VECTOR_ALLOC(sexptype, length, bytes, scref)
#define DYNTRACE_PROBE_EVAL_ENTRY(e, rho)
#define DYNTRACE_PROBE_EVAL_EXIT(e, rho, retval)
#define DYNTRACE_PROBE_GC_ENTRY(size_needed)
#define DYNTRACE_PROBE_GC_EXIT(gc_count, vcells, ncells)
#define DYNTRACE_PROBE_GC_PROMISE_UNMARKED(promise)
#define DYNTRACE_PROBE_JUMP_CTXT(rho, val)
#define DYNTRACE_PROBE_NEW_ENVIRONMENT(rho)
#define DYNTRACE_PROBE_S3_GENERIC_ENTRY(generic, object)
#define DYNTRACE_PROBE_S3_GENERIC_EXIT(generic, object, retval)
#define DYNTRACE_PROBE_S3_DISPATCH_ENTRY(generic, clazz, method, object)
#define DYNTRACE_PROBE_S3_DISPATCH_EXIT(generic, clazz, method, object, retval)
#define DYNTRACE_PROBE_ENVIRONMENT_DEFINE_VAR(symbol, value, rho)
#define DYNTRACE_PROBE_ENVIRONMENT_ASSIGN_VAR(symbol, value, rho)
#define DYNTRACE_PROBE_ENVIRONMENT_REMOVE_VAR(symbol, rho)
#define DYNTRACE_PROBE_ENVIRONMENT_LOOKUP_VAR(symbol, value, rho)
#endif

/* ----------------------------------------------------------------------------
DYNTRACE TYPE DEFINITIONS
---------------------------------------------------------------------------- */

typedef struct {
  clock_t probe_begin;
  clock_t probe_end;
  clock_t probe_function_entry;
  clock_t probe_function_exit;
  clock_t probe_builtin_entry;
  clock_t probe_builtin_exit;
  clock_t probe_specialsxp_entry;
  clock_t probe_specialsxp_exit;
  clock_t probe_promise_created;
  clock_t probe_promise_force_entry;
  clock_t probe_promise_force_exit;
  clock_t probe_promise_value_lookup;
  clock_t probe_promise_expression_lookup;
  clock_t probe_error;
  clock_t probe_vector_alloc;
  clock_t probe_eval_entry;
  clock_t probe_eval_exit;
  clock_t probe_gc_entry;
  clock_t probe_gc_exit;
  clock_t probe_gc_promise_unmarked;
  clock_t probe_jump_ctxt;
  clock_t probe_new_environment;
  clock_t probe_S3_generic_entry;
  clock_t probe_S3_generic_exit;
  clock_t probe_S3_dispatch_entry;
  clock_t probe_S3_dispatch_exit;
  clock_t probe_environment_define_var;
  clock_t probe_environment_assign_var;
  clock_t probe_environment_remove_var;
  clock_t probe_environment_lookup_var;
  clock_t expression;
} execution_time_t;

typedef struct {
  unsigned int probe_begin;
  unsigned int probe_end;
  unsigned int probe_function_entry;
  unsigned int probe_function_exit;
  unsigned int probe_builtin_entry;
  unsigned int probe_builtin_exit;
  unsigned int probe_specialsxp_entry;
  unsigned int probe_specialsxp_exit;
  unsigned int probe_promise_created;
  unsigned int probe_promise_force_entry;
  unsigned int probe_promise_force_exit;
  unsigned int probe_promise_value_lookup;
  unsigned int probe_promise_expression_lookup;
  unsigned int probe_error;
  unsigned int probe_vector_alloc;
  unsigned int probe_eval_entry;
  unsigned int probe_eval_exit;
  unsigned int probe_gc_entry;
  unsigned int probe_gc_exit;
  unsigned int probe_gc_promise_unmarked;
  unsigned int probe_jump_ctxt;
  unsigned int probe_new_environment;
  unsigned int probe_S3_generic_entry;
  unsigned int probe_S3_generic_exit;
  unsigned int probe_S3_dispatch_entry;
  unsigned int probe_S3_dispatch_exit;
  unsigned int probe_environment_define_var;
  unsigned int probe_environment_assign_var;
  unsigned int probe_environment_remove_var;
  unsigned int probe_environment_lookup_var;
  unsigned int expression;
} execution_count_t;

typedef struct {
  const char *r_compile_pkgs;
  const char *r_disable_bytecode;
  const char *r_enable_jit;
  const char *r_keep_pkg_source;
} environment_variables_t;

typedef struct {
  execution_time_t execution_time;
  execution_count_t execution_count;
  environment_variables_t environment_variables;
  const char *begin_datetime;
  const char *end_datetime;
} dyntracing_context_t;

typedef struct {
  void *dyntracer_context;
  dyntracing_context_t *dyntracing_context;
} dyntrace_context_t;

typedef struct {

  /***************************************************************************
  Fires when the tracer starts. Not an interpreter hook.
  Look for DYNTRACE_PROBE_BEGIN(...) in
  - src/main/dyntrace.c
  ***************************************************************************/
  void (*probe_begin)(dyntrace_context_t *dyntrace_context, const SEXP prom);

  /***************************************************************************
  Fires when the tracer ends (after the traced call returns).
  Not an interpreter hook.
  Look for DYNTRACE_PROBE_END(...) in
  - src/main/dyntrace.c
  ***************************************************************************/
  void (*probe_end)(dyntrace_context_t *dyntrace_context);

  /***************************************************************************
  Fires on every R function call.
  Look for DYNTRACE_PROBE_FUNCTION_ENTRY(...) in
  - src/main/eval.c
  ***************************************************************************/
  void (*probe_function_entry)(dyntrace_context_t *dyntrace_context,
                               const SEXP call, const SEXP op, const SEXP rho);

  /***************************************************************************
  Fires after every R function call.
  Look for DYNTRACE_PROBE_FUNCTION_EXIT(...) in
  - src/main/eval.c
  ***************************************************************************/
  void (*probe_function_exit)(dyntrace_context_t *dyntrace_context,
                              const SEXP call, const SEXP op, const SEXP rho,
                              const SEXP retval);

  /***************************************************************************
  Fires on every BUILTINSXP call.
  Look for DYNTRACE_PROBE_BUILTIN_ENTRY(...) in
  - src/main/eval.c
  ***************************************************************************/
  void (*probe_builtin_entry)(dyntrace_context_t *dyntrace_context,
                              const SEXP call, const SEXP op, const SEXP rho);

  /***************************************************************************
  Fires after every BUILTINSXP call.
  Look for DYNTRACE_PROBE_BUILTIN_EXIT(...) in
  - src/main/eval.c
  ***************************************************************************/
  void (*probe_builtin_exit)(dyntrace_context_t *dyntrace_context,
                             const SEXP call, const SEXP op, const SEXP rho,
                             const SEXP retval);

  /***************************************************************************
  Fires on every SPECIALSXP call.
  Look for DYNTRACE_PROBE_SPECIALSXP_ENTRY(...) in
  - src/main/eval.c
  ***************************************************************************/
  void (*probe_specialsxp_entry)(dyntrace_context_t *dyntrace_context,
                                 const SEXP call, const SEXP op,
                                 const SEXP rho);

  /***************************************************************************
  Fires after every SPECIALSXP call.
  Look for DYNTRACE_PROBE_SPECIALSXP_EXIT(...) in
  - in src/main/eval.c
  ***************************************************************************/
  void (*probe_specialsxp_exit)(dyntrace_context_t *dyntrace_context,
                                const SEXP call, const SEXP op, const SEXP rho,
                                const SEXP retval);

  /***************************************************************************
  Fires on promise allocation.
  Look for DYNTRACE_PROBE_PROMISE_CREATED(...) in
  - src/main/memory.c
  ***************************************************************************/
  void (*probe_promise_created)(dyntrace_context_t *dyntrace_context,
                                const SEXP prom, const SEXP rho);

  /***************************************************************************
  Fires when a promise is forced (accessed for the first time).
  Look for DYNTRACE_PROBE_PROMISE_FORCE_ENTRY(...) in
  - src/main/eval.c
  ***************************************************************************/
  void (*probe_promise_force_entry)(dyntrace_context_t *dyntrace_context,
                                    const SEXP promise);

  /***************************************************************************
  Fires right after a promise is forced.
  Look for DYNTRACE_PROBE_PROMISE_FORCE_EXIT(...) in
  - src/main/eval.c
  ***************************************************************************/
  void (*probe_promise_force_exit)(dyntrace_context_t *dyntrace_context,
                                   const SEXP promise);

  /***************************************************************************
  Fires when a promise is accessed but already forced.
  Look for DYNTRACE_PROBE_PROMISE_VALUE_LOOKUP(...) in
  - src/main/eval.c
  ***************************************************************************/
  void (*probe_promise_value_lookup)(dyntrace_context_t *dyntrace_context,
                                     const SEXP promise);

  /***************************************************************************
  Fires when the expression inside a promise is accessed.
  Look for DYNTRACE_PROBE_PROMISE_EXPRESSION_LOOKUP(...) in
  - src/main/coerce.c
  ***************************************************************************/
  void (*probe_promise_expression_lookup)(dyntrace_context_t *dyntrace_context,
                                          const SEXP promise);

  /***************************************************************************
  Look for DYNTRACE_PROBE_ERROR(...) in
  - src/main/errors.c
  ***************************************************************************/
  void (*probe_error)(dyntrace_context_t *dyntrace_context, const SEXP call,
                      const char *message);

  /***************************************************************************
  Look for DYNTRACE_PROBE_VECTOR_ALLOC(...) in
  - src/main/memory.c
  ***************************************************************************/
  void (*probe_vector_alloc)(dyntrace_context_t *dyntrace_context, int sexptype,
                             long length, long bytes, const char *srcref);

  /***************************************************************************
  Look for DYNTRACE_PROBE_EVAL_ENTRY(...) in
  - src/main/eval.c
  ***************************************************************************/
  void (*probe_eval_entry)(dyntrace_context_t *dyntrace_context, SEXP e,
                           SEXP rho);

  /***************************************************************************
  Look for DYNTRACE_PROBE_EVAL_EXIT(...) in
  - src/main/eval.c
  ***************************************************************************/
  void (*probe_eval_exit)(dyntrace_context_t *dyntrace_context, SEXP e,
                          SEXP rho, SEXP retval);

  /***************************************************************************
  Look for DYNTRACE_PROBE_GC_ENTRY(...) in
  - src/main/memory.c
  ***************************************************************************/
  void (*probe_gc_entry)(dyntrace_context_t *dyntrace_context,
                         R_size_t size_needed);

  /***************************************************************************
  Look for DYNTRACE_PROBE_GC_EXIT(...) in
  - src/main/memory.c
  ***************************************************************************/
  void (*probe_gc_exit)(dyntrace_context_t *dyntrace_context, int gc_count,
                        double vcells, double ncells);

  /***************************************************************************
  Fires when a promise gets garbage collected
  Look for DYNTRACE_PROBE_GC_PROMISE_UNMARKED(...) in
  - src/main/memory.c
  ***************************************************************************/
  void (*probe_gc_promise_unmarked)(dyntrace_context_t *dyntrace_context,
                                    const SEXP promise);

  /***************************************************************************
  Fires when the interpreter is about to longjump into a different context.
  Parameter rho is the target environment.
  Look for DYNTRACE_PROBE_JUMP_CTXT(...) in
  - src/main/context.c
  ***************************************************************************/
  void (*probe_jump_ctxt)(dyntrace_context_t *dyntrace_context, const SEXP rho,
                          const SEXP val);

  /***************************************************************************
  Fires when the interpreter creates a new environment.
  Parameter rho is the newly created environment.
  Look for DYNTRACE_PROBE_NEW_ENVIRONMENT(..) in
  - src/main/memory.c
  ***************************************************************************/
  void (*probe_new_environment)(dyntrace_context_t *dyntrace_context,
                                const SEXP rho);

  /***************************************************************************
  Look for DYNTRACE_PROBE_S3_GENERIC_ENTRY(...) in
  - src/main/objects.c
  ***************************************************************************/
  void (*probe_S3_generic_entry)(dyntrace_context_t *dyntrace_context,
                                 const char *generic, const SEXP object);

  /***************************************************************************
  Look for DYNTRACE_PROBE_S3_GENERIC_EXIT(...) in
  - src/main/objects.c
  ***************************************************************************/
  void (*probe_S3_generic_exit)(dyntrace_context_t *dyntrace_context,
                                const char *generic, const SEXP object,
                                const SEXP retval);

  /***************************************************************************
  Look for DYNTRACE_PROBE_S3_DISPATCH_ENTRY(...) in
  - src/main/objects.c
  ***************************************************************************/
  void (*probe_S3_dispatch_entry)(dyntrace_context_t *dyntrace_context,
                                  const char *generic, const char *clazz,
                                  const SEXP method, const SEXP object);

  /***************************************************************************
  Look for DYNTRACE_PROBE_S3_DISPATCH_EXIT(...) in
  - src/main/objects.c
  ***************************************************************************/
  void (*probe_S3_dispatch_exit)(dyntrace_context_t *dyntrace_context,
                                 const char *generic, const char *clazz,
                                 const SEXP method, const SEXP object,
                                 const SEXP retval);

  /***************************************************************************
  Look for DYNTRACE_PROBE_ENVIRONMENT_DEFINE_VAR(...) in
  - src/main/envir.c
  ***************************************************************************/
  void (*probe_environment_define_var)(dyntrace_context_t *dyntrace_context,
                                       SEXP symbol, SEXP value, SEXP rho);

  /***************************************************************************
  Look for DYNTRACE_PROBE_ENVIRONMENT_ASSIGN_VAR(...) in
  - src/main/envir.c
  ***************************************************************************/
  void (*probe_environment_assign_var)(dyntrace_context_t *dyntrace_context,
                                       SEXP symbol, SEXP value, SEXP rho);

  /***************************************************************************
  Look for DYNTRACE_PROBE_ENVIRONMENT_REMOVE_VAR(...) in
  - src/main/envir.c
  ***************************************************************************/
  void (*probe_environment_remove_var)(dyntrace_context_t *dyntrace_context,
                                       SEXP symbol, SEXP rho);

  /***************************************************************************
  Look for DYNTRACE_PROBE_ENVIRONMENT_LOOKUP_VAR(...) in
  - src/main/envir.c
  ***************************************************************************/
  void (*probe_environment_lookup_var)(dyntrace_context_t *dyntrace_context,
                                       SEXP symbol, SEXP value, SEXP rho);
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
// stopwatch for measuring execution time
extern clock_t dyntrace_stopwatch;
// flag for checking if we are in privileged mode
extern int dyntrace_privileged_mode_flag;

SEXP do_dyntrace(SEXP call, SEXP op, SEXP args, SEXP rho);
int dyntrace_is_active();
int dyntrace_should_probe();
int dyntrace_dyntracer_is_active();
int dyntrace_dyntracer_probe_is_active();
void dyntrace_disable_garbage_collector();
void dyntrace_reinstate_garbage_collector();
void dyntrace_enable_privileged_mode();
void dyntrace_disable_privileged_mode();
int dyntrace_is_priviliged_mode();
clock_t dyntrace_reset_stopwatch();
dyntracer_t *dyntracer_from_sexp(SEXP dyntracer_sexp);
SEXP dyntracer_to_sexp(dyntracer_t *dyntracer, const char *classname);
dyntracer_t *dyntracer_replace_sexp(SEXP dyntracer_sexp,
                                    dyntracer_t *new_dyntracer);
SEXP dyntracer_destroy_sexp(SEXP dyntracer_sexp,
                            void (*destroy_dyntracer)(dyntracer_t *dyntracer));

// ----------------------------------------------------------------------------
// helpers
// ----------------------------------------------------------------------------

#define CHKSTR(s) ((s) == NULL ? "<unknown>" : (s))
SEXP get_named_list_element(const SEXP list, const char *name);
char *serialize_sexp(SEXP s);
const char *get_string(SEXP sexp);
#ifdef __cplusplus
}
#endif

#endif /* __DYNTRACE_H__ */
