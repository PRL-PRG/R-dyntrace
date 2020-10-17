#ifndef __DYNTRACE_H__
#define __DYNTRACE_H__

#include <Rinternals.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DYNTRACE_PROBE_HEADER(callback_name)                               \
    if (dyntrace_is_enabled() &&                                           \
        dyntrace_active_dyntracer != NULL &&                               \
        dyntrace_active_dyntracer->callback.callback_name != NULL) {       \
//        if (dyntrace_active_dyntracer_callback_name != NULL) {             \
//            dyntrace_log_error("[NESTED HOOK EXECUTION] - %s triggers %s", \
//                               dyntrace_active_dyntracer_callback_name,    \
//                               #callback_name);                            \
//        }                                                                  \
//        dyntrace_active_dyntracer_callback_name = #callback_name;

#define DYNTRACE_PROBE_FOOTER(callback_name)        \
    dyntrace_active_dyntracer_callback_name = NULL; \
    }

#define DYNTRACE_PROBE_DYNTRACE_ENTRY(expression, environment) \
    DYNTRACE_PROBE_HEADER(dyntrace_entry);                     \
    PROTECT(expression);                                       \
    PROTECT(environment);                                      \
    dyntrace_active_dyntracer->callback.dyntrace_entry(        \
        dyntrace_active_dyntracer, expression, environment);   \
    UNPROTECT(2);                                              \
    DYNTRACE_PROBE_FOOTER(dyntrace_entry);

#define DYNTRACE_PROBE_DYNTRACE_EXIT(expression, environment, result, error) \
    DYNTRACE_PROBE_HEADER(dyntrace_exit);                                    \
    PROTECT(expression);                                                     \
    PROTECT(environment);                                                    \
    PROTECT(result);                                                         \
    dyntrace_active_dyntracer->callback.dyntrace_exit(                       \
        dyntrace_active_dyntracer, expression, environment, result, error);  \
    UNPROTECT(3);                                                            \
    DYNTRACE_PROBE_FOOTER(dyntrace_exit);

#define DYNTRACE_PROBE_DESERIALIZE_OBJECT(object)           \
    DYNTRACE_PROBE_HEADER(deserialize_object);              \
    PROTECT(object);                                        \
    dyntrace_active_dyntracer->callback.deserialize_object( \
        dyntrace_active_dyntracer, object);                 \
    UNPROTECT(1);                                           \
    DYNTRACE_PROBE_FOOTER(deserialize_object);

#define DYNTRACE_PROBE_CLOSURE_ARGUMENT_LIST_CREATION_ENTRY(                  \
    formals, actuals, parent_rho)                                             \
    DYNTRACE_PROBE_HEADER(closure_argument_list_creation_entry);              \
    PROTECT(formals);                                                         \
    PROTECT(actuals);                                                         \
    PROTECT(parent_rho);                                                      \
    dyntrace_active_dyntracer->callback.closure_argument_list_creation_entry( \
        dyntrace_active_dyntracer, formals, actuals, parent_rho);             \
    UNPROTECT(3);                                                             \
    DYNTRACE_PROBE_FOOTER(closure_argument_list_creation_entry);

#define DYNTRACE_PROBE_CLOSURE_ARGUMENT_LIST_CREATION_EXIT(rho)              \
    DYNTRACE_PROBE_HEADER(closure_argument_list_creation_exit);              \
    PROTECT(rho);                                                            \
    dyntrace_active_dyntracer->callback.closure_argument_list_creation_exit( \
        dyntrace_active_dyntracer, rho);                                     \
    UNPROTECT(1);                                                            \
    DYNTRACE_PROBE_FOOTER(closure_argument_list_creation_exit);

#define DYNTRACE_PROBE_CLOSURE_ENTRY(call, op, args, rho, dispatch) \
    DYNTRACE_PROBE_HEADER(closure_entry);                           \
    PROTECT(call);                                                  \
    PROTECT(op);                                                    \
    PROTECT(args);                                                  \
    PROTECT(rho);                                                   \
    dyntrace_active_dyntracer->callback.closure_entry(              \
        dyntrace_active_dyntracer, call, op, args, rho, dispatch);  \
    UNPROTECT(4);                                                   \
    DYNTRACE_PROBE_FOOTER(closure_entry);

#define DYNTRACE_PROBE_CLOSURE_EXIT(call, op, args, rho, dispatch, retval) \
    DYNTRACE_PROBE_HEADER(closure_exit);                                   \
    PROTECT(call);                                                         \
    PROTECT(op);                                                           \
    PROTECT(args);                                                         \
    PROTECT(rho);                                                          \
    PROTECT(retval);                                                       \
    dyntrace_active_dyntracer->callback.closure_exit(                      \
        dyntrace_active_dyntracer, call, op, args, rho, dispatch, retval); \
    UNPROTECT(5);                                                          \
    DYNTRACE_PROBE_FOOTER(closure_exit);

#define DYNTRACE_PROBE_BUILTIN_ENTRY(call, op, args, rho, dispatch) \
    DYNTRACE_PROBE_HEADER(builtin_entry);                           \
    PROTECT(call);                                                  \
    PROTECT(op);                                                    \
    PROTECT(args);                                                  \
    PROTECT(rho);                                                   \
    dyntrace_active_dyntracer->callback.builtin_entry(              \
        dyntrace_active_dyntracer, call, op, args, rho, dispatch);  \
    UNPROTECT(4);                                                   \
    DYNTRACE_PROBE_FOOTER(builtin_entry);

#define DYNTRACE_PROBE_BUILTIN_EXIT(                  \
    call, op, args, rho, dispatch, return_value)      \
    DYNTRACE_PROBE_HEADER(builtin_exit);              \
    PROTECT(call);                                    \
    PROTECT(op);                                      \
    PROTECT(args);                                    \
    PROTECT(rho);                                     \
    PROTECT(return_value);                            \
    dyntrace_active_dyntracer->callback.builtin_exit( \
        dyntrace_active_dyntracer,                    \
        call,                                         \
        op,                                           \
        args,                                         \
        rho,                                          \
        dispatch,                                     \
        return_value);                                \
    UNPROTECT(5);                                     \
    DYNTRACE_PROBE_FOOTER(builtin_exit);

#define DYNTRACE_PROBE_SPECIAL_ENTRY(call, op, args, rho, dispatch) \
    DYNTRACE_PROBE_HEADER(special_entry);                           \
    PROTECT(call);                                                  \
    PROTECT(op);                                                    \
    PROTECT(args);                                                  \
    PROTECT(rho);                                                   \
    dyntrace_active_dyntracer->callback.special_entry(              \
        dyntrace_active_dyntracer, call, op, args, rho, dispatch);  \
    UNPROTECT(4);                                                   \
    DYNTRACE_PROBE_FOOTER(special_entry);

#define DYNTRACE_PROBE_SPECIAL_EXIT(                  \
    call, op, args, rho, dispatch, return_value)      \
    DYNTRACE_PROBE_HEADER(special_exit);              \
    PROTECT(call);                                    \
    PROTECT(op);                                      \
    PROTECT(args);                                    \
    PROTECT(rho);                                     \
    PROTECT(return_value);                            \
    dyntrace_active_dyntracer->callback.special_exit( \
        dyntrace_active_dyntracer,                    \
        call,                                         \
        op,                                           \
        args,                                         \
        rho,                                          \
        dispatch,                                     \
        return_value);                                \
    UNPROTECT(5);                                     \
    DYNTRACE_PROBE_FOOTER(special_exit);

#define DYNTRACE_PROBE_SUBSTITUTE_CALL(                  \
    expression, environment, rho, return_value)          \
    DYNTRACE_PROBE_HEADER(substitute_call);              \
    PROTECT(expression);                                 \
    PROTECT(environment);                                \
    PROTECT(rho);                                        \
    PROTECT(return_value);                               \
    dyntrace_active_dyntracer->callback.substitute_call( \
        dyntrace_active_dyntracer,                       \
        expression,                                      \
        environment,                                     \
        rho,                                             \
        return_value);                                   \
    UNPROTECT(4);                                        \
    DYNTRACE_PROBE_FOOTER(substitute_call);

#define DYNTRACE_PROBE_ASSIGNMENT_CALL(                        \
    call, op, assignment_type, lhs, rhs, assign_env, eval_env) \
    DYNTRACE_PROBE_HEADER(assignment_call);                    \
    PROTECT(call);                                             \
    PROTECT(op);                                               \
    PROTECT(lhs);                                              \
    PROTECT(rhs);                                              \
    PROTECT(assign_env);                                       \
    PROTECT(eval_env);                                         \
    dyntrace_active_dyntracer->callback.assignment_call(       \
        dyntrace_active_dyntracer,                             \
        call,                                                  \
        op,                                                    \
        assignment_type,                                       \
        lhs,                                                   \
        rhs,                                                   \
        assign_env,                                            \
        eval_env);                                             \
    UNPROTECT(6);                                              \
    DYNTRACE_PROBE_FOOTER(assignment_call);

#define DYNTRACE_PROBE_PROMISE_FORCE_ENTRY(promise)          \
    DYNTRACE_PROBE_HEADER(promise_force_entry);              \
    PROTECT(promise);                                        \
    dyntrace_active_dyntracer->callback.promise_force_entry( \
        dyntrace_active_dyntracer, promise);                 \
    UNPROTECT(1);                                            \
    DYNTRACE_PROBE_FOOTER(promise_force_entry);

#define DYNTRACE_PROBE_PROMISE_FORCE_EXIT(promise)          \
    DYNTRACE_PROBE_HEADER(promise_force_exit);              \
    PROTECT(promise);                                       \
    dyntrace_active_dyntracer->callback.promise_force_exit( \
        dyntrace_active_dyntracer, promise);                \
    UNPROTECT(1);                                           \
    DYNTRACE_PROBE_FOOTER(promise_force_exit);

#define DYNTRACE_PROBE_PROMISE_VALUE_LOOKUP(promise)          \
    DYNTRACE_PROBE_HEADER(promise_value_lookup);              \
    PROTECT(promise);                                         \
    dyntrace_active_dyntracer->callback.promise_value_lookup( \
        dyntrace_active_dyntracer, promise);                  \
    UNPROTECT(1);                                             \
    DYNTRACE_PROBE_FOOTER(promise_value_lookup);

#define DYNTRACE_PROBE_PROMISE_VALUE_ASSIGN(promise, value)   \
    DYNTRACE_PROBE_HEADER(promise_value_assign);              \
    PROTECT(promise);                                         \
    PROTECT(value);                                           \
    dyntrace_active_dyntracer->callback.promise_value_assign( \
        dyntrace_active_dyntracer, promise, value);           \
    UNPROTECT(2);                                             \
    DYNTRACE_PROBE_FOOTER(promise_value_assign);

#define DYNTRACE_PROBE_PROMISE_EXPRESSION_LOOKUP(promise)          \
    DYNTRACE_PROBE_HEADER(promise_expression_lookup);              \
    PROTECT(promise);                                              \
    dyntrace_active_dyntracer->callback.promise_expression_lookup( \
        dyntrace_active_dyntracer, promise);                       \
    UNPROTECT(1);                                                  \
    DYNTRACE_PROBE_FOOTER(promise_expression_lookup);

#define DYNTRACE_PROBE_PROMISE_EXPRESSION_ASSIGN(promise, expression) \
    DYNTRACE_PROBE_HEADER(promise_expression_assign);                 \
    PROTECT(promise);                                                 \
    PROTECT(expression);                                              \
    dyntrace_active_dyntracer->callback.promise_expression_assign(    \
        dyntrace_active_dyntracer, promise, expression);              \
    UNPROTECT(2);                                                     \
    DYNTRACE_PROBE_FOOTER(promise_expression_assign);

#define DYNTRACE_PROBE_PROMISE_ENVIRONMENT_LOOKUP(promise)          \
    DYNTRACE_PROBE_HEADER(promise_environment_lookup);              \
    PROTECT((promise));                                             \
    dyntrace_active_dyntracer->callback.promise_environment_lookup( \
        dyntrace_active_dyntracer, promise);                        \
    UNPROTECT(1);                                                   \
    DYNTRACE_PROBE_FOOTER(promise_environment_lookup);

#define DYNTRACE_PROBE_PROMISE_ENVIRONMENT_ASSIGN(promise, environment) \
    DYNTRACE_PROBE_HEADER(promise_environment_assign);                  \
    PROTECT(promise);                                                   \
    PROTECT(environment);                                               \
    dyntrace_active_dyntracer->callback.promise_environment_assign(     \
        dyntrace_active_dyntracer, promise, environment);               \
    UNPROTECT(2);                                                       \
    DYNTRACE_PROBE_FOOTER(promise_environment_assign);

#define DYNTRACE_PROBE_PROMISE_SUBSTITUTE(promise)          \
    DYNTRACE_PROBE_HEADER(promise_substitute);              \
    PROTECT(promise);                                       \
    dyntrace_active_dyntracer->callback.promise_substitute( \
        dyntrace_active_dyntracer, promise);                \
    UNPROTECT(1);                                           \
    DYNTRACE_PROBE_FOOTER(promise_substitute);

#define DYNTRACE_PROBE_EVAL_ENTRY(expression, rho)   \
    DYNTRACE_PROBE_HEADER(eval_entry);               \
    PROTECT(expression);                             \
    PROTECT(rho);                                    \
    dyntrace_active_dyntracer->callback.eval_entry(  \
        dyntrace_active_dyntracer, expression, rho); \
    UNPROTECT(2);                                    \
    DYNTRACE_PROBE_FOOTER(eval_entry);

#define DYNTRACE_PROBE_EVAL_EXIT(expression, rho, return_value)    \
    DYNTRACE_PROBE_HEADER(eval_exit);                              \
    PROTECT(expression);                                           \
    PROTECT(rho);                                                  \
    PROTECT(return_value);                                         \
    dyntrace_active_dyntracer->callback.eval_exit(                 \
        dyntrace_active_dyntracer, expression, rho, return_value); \
    UNPROTECT(3);                                                  \
    DYNTRACE_PROBE_FOOTER(eval_exit);

#define DYNTRACE_PROBE_GC_ENTRY(size_needed)                                \
    DYNTRACE_PROBE_HEADER(gc_entry);                                        \
    dyntrace_active_dyntracer->callback.gc_entry(dyntrace_active_dyntracer, \
                                                 size_needed);              \
    DYNTRACE_PROBE_FOOTER(gc_entry);

#define DYNTRACE_PROBE_GC_EXIT(gc_count)                                   \
    DYNTRACE_PROBE_HEADER(gc_exit);                                        \
    dyntrace_active_dyntracer->callback.gc_exit(dyntrace_active_dyntracer, \
                                                gc_count);                 \
    DYNTRACE_PROBE_FOOTER(gc_exit);

#define DYNTRACE_PROBE_GC_UNMARK(object)                                     \
    DYNTRACE_PROBE_HEADER(gc_unmark);                                        \
    PROTECT(object);                                                         \
    dyntrace_active_dyntracer->callback.gc_unmark(dyntrace_active_dyntracer, \
                                                  object);                   \
    UNPROTECT(1);                                                            \
    DYNTRACE_PROBE_FOOTER(gc_unmark);

#define DYNTRACE_PROBE_GC_ALLOCATE(object)                                     \
    DYNTRACE_PROBE_HEADER(gc_allocate);                                        \
    PROTECT(object);                                                           \
    dyntrace_active_dyntracer->callback.gc_allocate(dyntrace_active_dyntracer, \
                                                    object);                   \
    UNPROTECT(1);                                                              \
    DYNTRACE_PROBE_FOOTER(gc_allocate);

#define DYNTRACE_PROBE_CONTEXT_ENTRY(context)          \
    DYNTRACE_PROBE_HEADER(context_entry);              \
    dyntrace_active_dyntracer->callback.context_entry( \
        dyntrace_active_dyntracer, context);           \
    DYNTRACE_PROBE_FOOTER(context_entry);

#define DYNTRACE_PROBE_CONTEXT_EXIT(context)          \
    DYNTRACE_PROBE_HEADER(context_exit);              \
    dyntrace_active_dyntracer->callback.context_exit( \
        dyntrace_active_dyntracer, context);          \
    DYNTRACE_PROBE_FOOTER(context_end);

#define DYNTRACE_PROBE_CONTEXT_JUMP(context, return_value, restart) \
    DYNTRACE_PROBE_HEADER(context_jump);                            \
    PROTECT(return_value);                                          \
    dyntrace_active_dyntracer->callback.context_jump(               \
        dyntrace_active_dyntracer, context, return_value, restart); \
    UNPROTECT(1);                                                   \
    DYNTRACE_PROBE_FOOTER(context_jump);

#define DYNTRACE_PROBE_S3_GENERIC_ENTRY(generic, generic_method, object) \
    DYNTRACE_PROBE_HEADER(S3_generic_entry);                             \
    PROTECT(generic_method);                                             \
    PROTECT(object);                                                     \
    dyntrace_active_dyntracer->callback.S3_generic_entry(                \
        dyntrace_active_dyntracer, generic, generic_method, object);     \
    UNPROTECT(2);                                                        \
    DYNTRACE_PROBE_FOOTER(S3_generic_entry);

#define DYNTRACE_PROBE_S3_GENERIC_EXIT(                                      \
    generic, generic_method, object, retval)                                 \
    DYNTRACE_PROBE_HEADER(S3_generic_exit);                                  \
    PROTECT(generic_method);                                                 \
    PROTECT(object);                                                         \
    PROTECT(retval);                                                         \
    dyntrace_active_dyntracer->callback.S3_generic_exit(                     \
        dyntrace_active_dyntracer, generic, generic_method, object, retval); \
    UNPROTECT(3);                                                            \
    DYNTRACE_PROBE_FOOTER(S3_generic_exit);

#define DYNTRACE_PROBE_S3_DISPATCH_ENTRY(                   \
    generic, cls, generic_method, specific_method, objects) \
    DYNTRACE_PROBE_HEADER(S3_dispatch_entry);               \
    PROTECT(cls);                                           \
    PROTECT(generic_method);                                \
    PROTECT(specific_method);                               \
    PROTECT(objects);                                       \
    dyntrace_active_dyntracer->callback.S3_dispatch_entry(  \
        dyntrace_active_dyntracer,                          \
        generic,                                            \
        cls,                                                \
        generic_method,                                     \
        specific_method,                                    \
        objects);                                           \
    UNPROTECT(4);                                           \
    DYNTRACE_PROBE_FOOTER(S3_dispatch_entry);

#define DYNTRACE_PROBE_S3_DISPATCH_EXIT(                                  \
    generic, cls, generic_method, specific_method, objects, return_value) \
    DYNTRACE_PROBE_HEADER(S3_dispatch_exit);                              \
    PROTECT(cls);                                                         \
    PROTECT(generic_method);                                              \
    PROTECT(specific_method);                                             \
    PROTECT(objects);                                                     \
    PROTECT(return_value);                                                \
    dyntrace_active_dyntracer->callback.S3_dispatch_exit(                 \
        dyntrace_active_dyntracer,                                        \
        generic,                                                          \
        cls,                                                              \
        generic_method,                                                   \
        specific_method,                                                  \
        objects,                                                          \
        return_value);                                                    \
    UNPROTECT(5);                                                         \
    DYNTRACE_PROBE_FOOTER(S3_dispatch_exit);

#define DYNTRACE_PROBE_S4_GENERIC_ENTRY(fname, env, fdef) \
    DYNTRACE_PROBE_HEADER(S4_generic_entry);              \
    PROTECT(fname);                                       \
    PROTECT(env);                                         \
    PROTECT(fdef);                                        \
    dyntrace_active_dyntracer->callback.S4_generic_entry( \
        dyntrace_active_dyntracer, fname, env, fdef);     \
    UNPROTECT(3);                                         \
    DYNTRACE_PROBE_FOOTER(S4_generic_entry);

#define DYNTRACE_PROBE_S4_GENERIC_EXIT(fname, env, fdef, return_value) \
    DYNTRACE_PROBE_HEADER(S4_generic_exit);                            \
    PROTECT(fname);                                                    \
    PROTECT(env);                                                      \
    PROTECT(fdef);                                                     \
    PROTECT(return_value);                                             \
    dyntrace_active_dyntracer->callback.S4_generic_exit(               \
        dyntrace_active_dyntracer, fname, env, fdef, return_value);    \
    UNPROTECT(4);                                                      \
    DYNTRACE_PROBE_FOOTER(S4_generic_exit);

#define DYNTRACE_PROBE_S4_DISPATCH_ENTRY(call, op, args, rho) \
    DYNTRACE_PROBE_HEADER(S4_dispatch_entry);                 \
    PROTECT(call);                                            \
    PROTECT(op);                                              \
    PROTECT(args);                                            \
    PROTECT(rho);                                             \
    dyntrace_active_dyntracer->callback.S4_dispatch_entry(    \
        dyntrace_active_dyntracer, call, op, args, rho);      \
    UNPROTECT(4);                                             \
    DYNTRACE_PROBE_FOOTER(S4_dispatch_entry);

#define DYNTRACE_PROBE_S4_DISPATCH_EXIT(call, op, args, rho, return_value) \
    DYNTRACE_PROBE_HEADER(S4_dispatch_exit);                               \
    PROTECT(call);                                                         \
    PROTECT(op);                                                           \
    PROTECT(args);                                                         \
    PROTECT(rho);                                                          \
    PROTECT(return_value);                                                 \
    dyntrace_active_dyntracer->callback.S4_dispatch_exit(                  \
        dyntrace_active_dyntracer, call, op, args, rho, return_value);     \
    UNPROTECT(5);                                                          \
    DYNTRACE_PROBE_FOOTER(S4_dispatch_exit);

#define DYNTRACE_PROBE_S4_DISPATCH_ARGUMENT(argument)         \
    DYNTRACE_PROBE_HEADER(S4_dispatch_argument);              \
    PROTECT(argument);                                        \
    dyntrace_active_dyntracer->callback.S4_dispatch_argument( \
        dyntrace_active_dyntracer, argument);                 \
    UNPROTECT(1);                                             \
    DYNTRACE_PROBE_FOOTER(S4_dispatch_argument);

#define DYNTRACE_PROBE_ENVIRONMENT_VARIABLE_DEFINE(symbol, value, rho) \
    DYNTRACE_PROBE_HEADER(environment_variable_define);                \
    PROTECT(symbol);                                                   \
    PROTECT(value);                                                    \
    PROTECT(rho);                                                      \
    dyntrace_active_dyntracer->callback.environment_variable_define(   \
        dyntrace_active_dyntracer, symbol, value, rho);                \
    UNPROTECT(3);                                                      \
    DYNTRACE_PROBE_FOOTER(environment_variable_define);

#define DYNTRACE_PROBE_ENVIRONMENT_VARIABLE_ASSIGN(symbol, value, rho) \
    DYNTRACE_PROBE_HEADER(environment_variable_assign);                \
    PROTECT(symbol);                                                   \
    PROTECT(value);                                                    \
    PROTECT(rho);                                                      \
    dyntrace_active_dyntracer->callback.environment_variable_assign(   \
        dyntrace_active_dyntracer, symbol, value, rho);                \
    UNPROTECT(3);                                                      \
    DYNTRACE_PROBE_FOOTER(environment_variable_assign);

#define DYNTRACE_PROBE_ENVIRONMENT_VARIABLE_REMOVE(symbol, rho)      \
    DYNTRACE_PROBE_HEADER(environment_variable_remove);              \
    PROTECT(symbol);                                                 \
    PROTECT(rho);                                                    \
    dyntrace_active_dyntracer->callback.environment_variable_remove( \
        dyntrace_active_dyntracer, symbol, rho);                     \
    UNPROTECT(2);                                                    \
    DYNTRACE_PROBE_FOOTER(environment_variable_remove);

#define DYNTRACE_PROBE_ENVIRONMENT_VARIABLE_LOOKUP(symbol, value, rho) \
    DYNTRACE_PROBE_HEADER(environment_variable_lookup);                \
    PROTECT(symbol);                                                   \
    PROTECT(value);                                                    \
    PROTECT(rho);                                                      \
    dyntrace_active_dyntracer->callback.environment_variable_lookup(   \
        dyntrace_active_dyntracer, symbol, value, rho);                \
    UNPROTECT(3);                                                      \
    DYNTRACE_PROBE_FOOTER(environment_variable_lookup);

#define DYNTRACE_PROBE_ENVIRONMENT_VARIABLE_EXISTS(symbol, rho)      \
    DYNTRACE_PROBE_HEADER(environment_variable_exists);              \
    PROTECT(symbol);                                                 \
    PROTECT(rho);                                                    \
    dyntrace_active_dyntracer->callback.environment_variable_exists( \
        dyntrace_active_dyntracer, symbol, rho);                     \
    UNPROTECT(2);                                                    \
    DYNTRACE_PROBE_FOOTER(environment_variable_exists);

#define DYNTRACE_PROBE_ENVIRONMENT_CONTEXT_SENSITIVE_PROMISE_EVAL_ENTRY(     \
    symbol, promise, rho)                                                    \
    DYNTRACE_PROBE_HEADER(environment_context_sensitive_promise_eval_entry); \
    PROTECT(symbol);                                                         \
    PROTECT(promise);                                                        \
    PROTECT(rho);                                                            \
    dyntrace_active_dyntracer->callback                                      \
        .environment_context_sensitive_promise_eval_entry(                   \
            dyntrace_active_dyntracer, symbol, promise, rho);                \
    UNPROTECT(3);                                                            \
    DYNTRACE_PROBE_FOOTER(environment_context_sensitive_promise_eval_entry);

#define DYNTRACE_PROBE_ENVIRONMENT_CONTEXT_SENSITIVE_PROMISE_EVAL_EXIT(     \
    symbol, promise, value, rho)                                            \
    DYNTRACE_PROBE_HEADER(environment_context_sensitive_promise_eval_exit); \
    PROTECT(symbol);                                                        \
    PROTECT(promise);                                                       \
    PROTECT(value);                                                         \
    PROTECT(rho);                                                           \
    dyntrace_active_dyntracer->callback                                     \
        .environment_context_sensitive_promise_eval_exit(                   \
            dyntrace_active_dyntracer, symbol, promise, value, rho);        \
    UNPROTECT(4);                                                           \
    DYNTRACE_PROBE_FOOTER(environment_context_sensitive_promise_eval_exit);

/* ----------------------------------------------------------------------------
   DYNTRACE TYPE DEFINITIONS
---------------------------------------------------------------------------- */

enum dyntrace_dispatch_t {
    DYNTRACE_DISPATCH_NONE,
    DYNTRACE_DISPATCH_S3,
    DYNTRACE_DISPATCH_S4
};

typedef enum dyntrace_dispatch_t dyntrace_dispatch_t;

enum dyntrace_assignment_t {
    DYNTRACE_ASSIGNMENT_ASSIGN,
    DYNTRACE_ASSIGNMENT_DEFINE,
    DYNTRACE_ASSIGNMENT_DEFINE_APPLY
};

typedef enum dyntrace_assignment_t dyntrace_assignment_t;

typedef struct dyntracer_t dyntracer_t;

typedef struct dyntracer_callback_t dyntracer_callback_t;

/***************************************************************************
Fires when the tracer starts. Not an interpreter callback.
***************************************************************************/
typedef void (*dyntrace_entry_callback_t)(dyntracer_t* dyntracer,
                                  SEXP expression,
                                  SEXP environment);

/***************************************************************************
Fires when the tracer ends. Not an interpreter callback.
***************************************************************************/
typedef void (*dyntrace_exit_callback_t)(dyntracer_t* dyntracer,
                                 SEXP expression,
                                 SEXP environment,
                                 SEXP result,
                                 int error);

/***************************************************************************
Fires when an object is unserialized
***************************************************************************/
typedef void (*deserialize_object_callback_t)(dyntracer_t* dyntracer, SEXP object);

/***************************************************************************
Fires before every closure call.
***************************************************************************/
typedef void (*closure_argument_list_creation_entry_callback_t)(dyntracer_t* dyntracer,
                                                        SEXP formals,
                                                        SEXP actuals,
                                                        SEXP parent_rho);

/***************************************************************************
Fires before every closure call.
***************************************************************************/
typedef void (*closure_argument_list_creation_exit_callback_t)(dyntracer_t* dyntracer,
                                                       SEXP rho);

/***************************************************************************
Fires on every closure call.
***************************************************************************/
typedef void (*closure_entry_callback_t)(dyntracer_t* dyntracer,
                                 const SEXP call,
                                 const SEXP op,
                                 const SEXP args,
                                 const SEXP rho,
                                 dyntrace_dispatch_t dispatch);

/***************************************************************************
Fires after every R closure call.
***************************************************************************/
typedef void (*closure_exit_callback_t)(dyntracer_t* dyntracer,
                                const SEXP call,
                                const SEXP op,
                                const SEXP args,
                                const SEXP rho,
                                dyntrace_dispatch_t dispatch,
                                const SEXP return_value);

/***************************************************************************
Fires on every builtin call.
***************************************************************************/
typedef void (*builtin_entry_callback_t)(dyntracer_t* dyntracer,
                                 const SEXP call,
                                 const SEXP op,
                                 const SEXP args,
                                 const SEXP rho,
                                 dyntrace_dispatch_t dispatch);

    // TODO: change order of dispatch
/***************************************************************************
Fires after every builtin call.
***************************************************************************/
typedef void (*builtin_exit_callback_t)(dyntracer_t* dyntracer,
                                const SEXP call,
                                const SEXP op,
                                const SEXP args,
                                const SEXP rho,
                                dyntrace_dispatch_t dispatch,
                                const SEXP return_value);

/***************************************************************************
Fires on every special call.
***************************************************************************/
typedef void (*special_entry_callback_t)(dyntracer_t* dyntracer,
                                 const SEXP call,
                                 const SEXP op,
                                 const SEXP args,
                                 const SEXP rho,
                                 dyntrace_dispatch_t dispatch);

/***************************************************************************
Fires after every special call.
***************************************************************************/
typedef void (*special_exit_callback_t)(dyntracer_t* dyntracer,
                                const SEXP call,
                                const SEXP op,
                                const SEXP args,
                                const SEXP rho,
                                dyntrace_dispatch_t dispatch,
                                const SEXP return_value);

/***************************************************************************
Fires when substitute is about to finish executing
***************************************************************************/
typedef void (*substitute_call_callback_t)(dyntracer_t* dyntracer,
                                   const SEXP expression,
                                   const SEXP environment,
                                   const SEXP rho,
                                   const SEXP return_value);

/***************************************************************************
  Fires when assignment is about to happen
  ***************************************************************************/
typedef void (*assignment_call_callback_t)(dyntracer_t* dyntracer,
                                   const SEXP call,
                                   const SEXP op,
                                   dyntrace_assignment_t assignment_type,
                                   const SEXP lhs,
                                   const SEXP rhs,
                                   const SEXP assign_env,
                                   const SEXP eval_env);

/***************************************************************************
Fires when a promise expression is evaluated.
***************************************************************************/
typedef void (*promise_force_entry_callback_t)(dyntracer_t* dyntracer,
                                       const SEXP promise);

/***************************************************************************
Fires after a promise expression is evaluated.
***************************************************************************/
typedef void (*promise_force_exit_callback_t)(dyntracer_t* dyntracer,
                                      const SEXP promise);

/***************************************************************************
Fires when the promise value is looked up.
***************************************************************************/
typedef void (*promise_value_lookup_callback_t)(dyntracer_t* dyntracer,
                                        const SEXP promise);

/***************************************************************************
Fires when the promise value is assigned.
***************************************************************************/
typedef void (*promise_value_assign_callback_t)(dyntracer_t* dyntracer,
                                        const SEXP promise,
                                        const SEXP value);

/***************************************************************************
Fires when the promise expression is looked up.
***************************************************************************/
typedef void (*promise_expression_lookup_callback_t)(dyntracer_t* dyntracer,
                                             const SEXP promise);

/***************************************************************************
Fires when the promise expression is assigned.
***************************************************************************/
typedef void (*promise_expression_assign_callback_t)(dyntracer_t* dyntracer,
                                             const SEXP promise,
                                             const SEXP expression);

/***************************************************************************
Fires when the promise environment is looked up.
***************************************************************************/
typedef void (*promise_environment_lookup_callback_t)(dyntracer_t* dyntracer,
                                              const SEXP promise);

/***************************************************************************
Fires when the promise environment is assigned.
***************************************************************************/
typedef void (*promise_environment_assign_callback_t)(dyntracer_t* dyntracer,
                                              const SEXP promise,
                                              const SEXP environment);

/***************************************************************************
Fires when the promise expression is looked up by substitute
***************************************************************************/
typedef void (*promise_substitute_callback_t)(dyntracer_t* dyntracer,
                                      const SEXP promise);

/***************************************************************************
Fires when the eval function is entered.
***************************************************************************/
typedef void (*eval_entry_callback_t)(dyntracer_t* dyntracer,
                              const SEXP expression,
                              const SEXP rho);

/***************************************************************************
Fires when the eval function is exited.
***************************************************************************/
typedef void (*eval_exit_callback_t)(dyntracer_t* dyntracer,
                             const SEXP expression,
                             const SEXP rho,
                             SEXP return_value);

/***************************************************************************
Fires when the garbage collector is entered.
***************************************************************************/
typedef void (*gc_entry_callback_t)(dyntracer_t* dyntracer, const size_t size_needed);

/***************************************************************************
Fires when the garbage collector is exited.
***************************************************************************/
typedef void (*gc_exit_callback_t)(dyntracer_t* dyntracer, int gc_count);

/***************************************************************************
Fires when an object gets garbage collected.
***************************************************************************/
typedef void (*gc_unmark_callback_t)(dyntracer_t* dyntracer, const SEXP object);

/***************************************************************************
Fires when an object gets allocated.
***************************************************************************/
typedef void (*gc_allocate_callback_t)(dyntracer_t* dyntracer, const SEXP object);

/***************************************************************************
Fires when a new context is entered.
***************************************************************************/
typedef void (*context_entry_callback_t)(dyntracer_t* dyntracer, const void* context);

/***************************************************************************
Fires when a context is exited.
***************************************************************************/
typedef void (*context_exit_callback_t)(dyntracer_t* dyntracer, const void* context);

/***************************************************************************
Fires when the interpreter is about to longjump into a different context.
***************************************************************************/
typedef void (*context_jump_callback_t)(dyntracer_t* dyntracer,
                                const void* context,
                                const SEXP return_value,
                                int restart);

/***************************************************************************
Fires when a generic S3 function is entered.
***************************************************************************/
typedef void (*S3_generic_entry_callback_t)(dyntracer_t* dyntracer,
                                    const char* generic,
                                    const SEXP generic_method,
                                    const SEXP object);

/***************************************************************************
Fires when a generic S3 function is exited.
***************************************************************************/
typedef void (*S3_generic_exit_callback_t)(dyntracer_t* dyntracer,
                                   const char* generic,
                                   const SEXP generic_method,
                                   const SEXP object,
                                   const SEXP retval);

/***************************************************************************
Fires when a S3 function is entered.
***************************************************************************/
typedef void (*S3_dispatch_entry_callback_t)(dyntracer_t* dyntracer,
                                     const char* generic,
                                     const SEXP cls,
                                     const SEXP generic_method,
                                     const SEXP specific_method,
                                     const SEXP objects);

/***************************************************************************
Fires when a S3 function is exited.
***************************************************************************/
typedef void (*S3_dispatch_exit_callback_t)(dyntracer_t* dyntracer,
                                    const char* generic,
                                    const SEXP cls,
                                    const SEXP generic_method,
                                    const SEXP specific_method,
                                    const SEXP return_value,
                                    const SEXP objects);

/***************************************************************************
Fires when S4 generic is entered
***************************************************************************/
typedef void (*S4_generic_entry_callback_t)(dyntracer_t* dyntracer,
                                    const SEXP fname,
                                    const SEXP env,
                                    const SEXP fdef);

/***************************************************************************
Fires when S4 generic is exited
***************************************************************************/
typedef void (*S4_generic_exit_callback_t)(dyntracer_t* dyntracer,
                                   const SEXP fname,
                                   const SEXP env,
                                   const SEXP fdef,
                                   const SEXP return_value);

/***************************************************************************
Fires when finding the class of the argument for S4 dispatch
***************************************************************************/
typedef void (*S4_dispatch_argument_callback_t)(dyntracer_t* dyntracer,
                                        const SEXP argument);

/***************************************************************************
Fires when a variable is defined in an environment.
***************************************************************************/
typedef void (*environment_variable_define_callback_t)(dyntracer_t* dyntracer,
                                               const SEXP symbol,
                                               const SEXP value,
                                               const SEXP rho);

/***************************************************************************
Fires when a variable is assigned in an environment.
***************************************************************************/
typedef void (*environment_variable_assign_callback_t)(dyntracer_t* dyntracer,
                                               const SEXP symbol,
                                               const SEXP value,
                                               const SEXP rho);

/***************************************************************************
Fires when a variable is removed from an environment.
***************************************************************************/
typedef void (*environment_variable_remove_callback_t)(dyntracer_t* dyntracer,
                                               const SEXP symbol,
                                               const SEXP rho);

/***************************************************************************
Fires when a variable is looked up in an environment.
***************************************************************************/
typedef void (*environment_variable_lookup_callback_t)(dyntracer_t* dyntracer,
                                               const SEXP symbol,
                                               const SEXP value,
                                               const SEXP rho);

/***************************************************************************
Fires when a variables is checked for existence in an environment.
***************************************************************************/
typedef void (*environment_variable_exists_callback_t)(dyntracer_t* dyntracer,
                                               const SEXP symbol,
                                               SEXP rho);

/***************************************************************************
Fires when a promise is about to be evaluated for context sensitive function
lookup
***************************************************************************/
typedef void (*environment_context_sensitive_promise_eval_entry_callback_t)(
    dyntracer_t* dyntracer,
    const SEXP symbol,
    SEXP promise,
    SEXP rho);

/***************************************************************************
Fires when a promise has been evaluated for context sensitive function
lookup
***************************************************************************/
typedef void (*environment_context_sensitive_promise_eval_exit_callback_t)(
    dyntracer_t* dyntracer,
    const SEXP symbol,
    SEXP promise,
    SEXP value,
    SEXP rho);

struct dyntracer_callback_t {
    dyntrace_entry_callback_t dyntrace_entry;
    dyntrace_exit_callback_t dyntrace_exit;
    deserialize_object_callback_t deserialize_object;
    closure_argument_list_creation_entry_callback_t
        closure_argument_list_creation_entry;
    closure_argument_list_creation_exit_callback_t
        closure_argument_list_creation_exit;
    closure_entry_callback_t closure_entry;
    closure_exit_callback_t closure_exit;
    builtin_entry_callback_t builtin_entry;
    builtin_exit_callback_t builtin_exit;
    special_entry_callback_t special_entry;
    special_exit_callback_t special_exit;
    substitute_call_callback_t substitute_call;
    assignment_call_callback_t assignment_call;
    promise_force_entry_callback_t promise_force_entry;
    promise_force_exit_callback_t promise_force_exit;
    promise_value_lookup_callback_t promise_value_lookup;
    promise_value_assign_callback_t promise_value_assign;
    promise_expression_lookup_callback_t promise_expression_lookup;
    promise_expression_assign_callback_t promise_expression_assign;
    promise_environment_lookup_callback_t promise_environment_lookup;
    promise_environment_assign_callback_t promise_environment_assign;
    promise_substitute_callback_t promise_substitute;
    eval_entry_callback_t eval_entry;
    eval_exit_callback_t eval_exit;
    gc_entry_callback_t gc_entry;
    gc_exit_callback_t gc_exit;
    gc_unmark_callback_t gc_unmark;
    gc_allocate_callback_t gc_allocate;
    context_entry_callback_t context_entry;
    context_exit_callback_t context_exit;
    context_jump_callback_t context_jump;
    S3_generic_entry_callback_t S3_generic_entry;
    S3_generic_exit_callback_t S3_generic_exit;
    S3_dispatch_entry_callback_t S3_dispatch_entry;
    S3_dispatch_exit_callback_t S3_dispatch_exit;
    S4_generic_entry_callback_t S4_generic_entry;
    S4_generic_exit_callback_t S4_generic_exit;
    S4_dispatch_argument_callback_t S4_dispatch_argument;
    environment_variable_define_callback_t environment_variable_define;
    environment_variable_assign_callback_t environment_variable_assign;
    environment_variable_remove_callback_t environment_variable_remove;
    environment_variable_lookup_callback_t environment_variable_lookup;
    environment_variable_exists_callback_t environment_variable_exists;
    environment_context_sensitive_promise_eval_entry_callback_t
        environment_context_sensitive_promise_eval_entry;
    environment_context_sensitive_promise_eval_exit_callback_t
        environment_context_sensitive_promise_eval_exit;
};

struct dyntracer_t {
    dyntracer_callback_t callback;
    void* data;
};

void dyntrace_enable();
void dyntrace_disable();
int dyntrace_is_enabled();

dyntracer_t* dyntracer_create(void *data);
void* dyntracer_destroy(dyntracer_t* dyntracer);

void dyntracer_set_data(dyntracer_t* dyntracer, void* data);
int dyntracer_has_data(dyntracer_t* dyntracer);
void* dyntracer_get_data(dyntracer_t* dyntracer);

int dyntracer_has_dyntrace_entry_callback(dyntracer_t* dyntracer);
int dyntracer_has_dyntrace_exit_callback(dyntracer_t* dyntracer);
int dyntracer_has_deserialize_object_callback(dyntracer_t* dyntracer);
int dyntracer_has_closure_argument_list_creation_entry_callback(dyntracer_t* dyntracer);
int dyntracer_has_closure_argument_list_creation_exit_callback(dyntracer_t* dyntracer);
int dyntracer_has_closure_entry_callback(dyntracer_t* dyntracer);
int dyntracer_has_closure_exit_callback(dyntracer_t* dyntracer);
int dyntracer_has_builtin_entry_callback(dyntracer_t* dyntracer);
int dyntracer_has_builtin_exit_callback(dyntracer_t* dyntracer);
int dyntracer_has_special_entry_callback(dyntracer_t* dyntracer);
int dyntracer_has_special_exit_callback(dyntracer_t* dyntracer);
int dyntracer_has_substitute_call_callback(dyntracer_t* dyntracer);
int dyntracer_has_assignment_call_callback(dyntracer_t* dyntracer);
int dyntracer_has_promise_force_entry_callback(dyntracer_t* dyntracer);
int dyntracer_has_promise_force_exit_callback(dyntracer_t* dyntracer);
int dyntracer_has_promise_value_lookup_callback(dyntracer_t* dyntracer);
int dyntracer_has_promise_value_assign_callback(dyntracer_t* dyntracer);
int dyntracer_has_promise_expression_lookup_callback(dyntracer_t* dyntracer);
int dyntracer_has_promise_expression_assign_callback(dyntracer_t* dyntracer);
int dyntracer_has_promise_environment_lookup_callback(dyntracer_t* dyntracer);
int dyntracer_has_promise_environment_assign_callback(dyntracer_t* dyntracer);
int dyntracer_has_promise_substitute_callback(dyntracer_t* dyntracer);
int dyntracer_has_eval_entry_callback(dyntracer_t* dyntracer);
int dyntracer_has_eval_exit_callback(dyntracer_t* dyntracer);
int dyntracer_has_gc_entry_callback(dyntracer_t* dyntracer);
int dyntracer_has_gc_exit_callback(dyntracer_t* dyntracer);
int dyntracer_has_gc_unmark_callback(dyntracer_t* dyntracer);
int dyntracer_has_gc_allocate_callback(dyntracer_t* dyntracer);
int dyntracer_has_context_entry_callback(dyntracer_t* dyntracer);
int dyntracer_has_context_exit_callback(dyntracer_t* dyntracer);
int dyntracer_has_context_jump_callback(dyntracer_t* dyntracer);
int dyntracer_has_S3_generic_entry_callback(dyntracer_t* dyntracer);
int dyntracer_has_S3_generic_exit_callback(dyntracer_t* dyntracer);
int dyntracer_has_S3_dispatch_entry_callback(dyntracer_t* dyntracer);
int dyntracer_has_S3_dispatch_exit_callback(dyntracer_t* dyntracer);
int dyntracer_has_S4_generic_entry_callback(dyntracer_t* dyntracer);
int dyntracer_has_S4_generic_exit_callback(dyntracer_t* dyntracer);
int dyntracer_has_S4_dispatch_argument_callback(dyntracer_t* dyntracer);
int dyntracer_has_environment_variable_define_callback(dyntracer_t* dyntracer);
int dyntracer_has_environment_variable_assign_callback(dyntracer_t* dyntracer);
int dyntracer_has_environment_variable_remove_callback(dyntracer_t* dyntracer);
int dyntracer_has_environment_variable_lookup_callback(dyntracer_t* dyntracer);
int dyntracer_has_environment_variable_exists_callback(dyntracer_t* dyntracer);
int dyntracer_has_environment_context_sensitive_promise_eval_entry_callback(
    dyntracer_t* dyntracer);
int dyntracer_has_environment_context_sensitive_promise_eval_exit_callback(
    dyntracer_t* dyntracer);

dyntrace_entry_callback_t
dyntracer_get_dyntrace_entry_callback(dyntracer_t* dyntracer);
dyntrace_exit_callback_t
dyntracer_get_dyntrace_exit_callback(dyntracer_t* dyntracer);
deserialize_object_callback_t
dyntracer_get_deserialize_object_callback(dyntracer_t* dyntracer);
closure_argument_list_creation_entry_callback_t
dyntracer_get_closure_argument_list_creation_entry_callback(
    dyntracer_t* dyntracer);
closure_argument_list_creation_exit_callback_t
dyntracer_get_closure_argument_list_creation_exit_callback(
    dyntracer_t* dyntracer);
closure_entry_callback_t
dyntracer_get_closure_entry_callback(dyntracer_t* dyntracer);
closure_exit_callback_t
dyntracer_get_closure_exit_callback(dyntracer_t* dyntracer);
builtin_entry_callback_t
dyntracer_get_builtin_entry_callback(dyntracer_t* dyntracer);
builtin_exit_callback_t
dyntracer_get_builtin_exit_callback(dyntracer_t* dyntracer);
special_entry_callback_t
dyntracer_get_special_entry_callback(dyntracer_t* dyntracer);
special_exit_callback_t
dyntracer_get_special_exit_callback(dyntracer_t* dyntracer);
substitute_call_callback_t
dyntracer_get_substitute_call_callback(dyntracer_t* dyntracer);
assignment_call_callback_t
dyntracer_get_assignment_call_callback(dyntracer_t* dyntracer);
promise_force_entry_callback_t
dyntracer_get_promise_force_entry_callback(dyntracer_t* dyntracer);
promise_force_exit_callback_t
dyntracer_get_promise_force_exit_callback(dyntracer_t* dyntracer);
promise_value_lookup_callback_t
dyntracer_get_promise_value_lookup_callback(dyntracer_t* dyntracer);
promise_value_assign_callback_t
dyntracer_get_promise_value_assign_callback(dyntracer_t* dyntracer);
promise_expression_lookup_callback_t
dyntracer_get_promise_expression_lookup_callback(dyntracer_t* dyntracer);
promise_expression_assign_callback_t
dyntracer_get_promise_expression_assign_callback(dyntracer_t* dyntracer);
promise_environment_lookup_callback_t
dyntracer_get_promise_environment_lookup_callback(dyntracer_t* dyntracer);
promise_environment_assign_callback_t
dyntracer_get_promise_environment_assign_callback(dyntracer_t* dyntracer);
promise_substitute_callback_t
dyntracer_get_promise_substitute_callback(dyntracer_t* dyntracer);
eval_entry_callback_t dyntracer_get_eval_entry_callback(dyntracer_t* dyntracer);
eval_exit_callback_t dyntracer_get_eval_exit_callback(dyntracer_t* dyntracer);
gc_entry_callback_t dyntracer_get_gc_entry_callback(dyntracer_t* dyntracer);
gc_exit_callback_t dyntracer_get_gc_exit_callback(dyntracer_t* dyntracer);
gc_unmark_callback_t dyntracer_get_gc_unmark_callback(dyntracer_t* dyntracer);
gc_allocate_callback_t
dyntracer_get_gc_allocate_callback(dyntracer_t* dyntracer);
context_entry_callback_t
dyntracer_get_context_entry_callback(dyntracer_t* dyntracer);
context_exit_callback_t
dyntracer_get_context_exit_callback(dyntracer_t* dyntracer);
context_jump_callback_t
dyntracer_get_context_jump_callback(dyntracer_t* dyntracer);
S3_generic_entry_callback_t
dyntracer_get_S3_generic_entry_callback(dyntracer_t* dyntracer);
S3_generic_exit_callback_t
dyntracer_get_S3_generic_exit_callback(dyntracer_t* dyntracer);
S3_dispatch_entry_callback_t
dyntracer_get_S3_dispatch_entry_callback(dyntracer_t* dyntracer);
S3_dispatch_exit_callback_t
dyntracer_get_S3_dispatch_exit_callback(dyntracer_t* dyntracer);
S4_generic_entry_callback_t
dyntracer_get_S4_generic_entry_callback(dyntracer_t* dyntracer);
S4_generic_exit_callback_t
dyntracer_get_S4_generic_exit_callback(dyntracer_t* dyntracer);
S4_dispatch_argument_callback_t
dyntracer_get_S4_dispatch_argument_callback(dyntracer_t* dyntracer);
environment_variable_define_callback_t
dyntracer_get_environment_variable_define_callback(dyntracer_t* dyntracer);
environment_variable_assign_callback_t
dyntracer_get_environment_variable_assign_callback(dyntracer_t* dyntracer);
environment_variable_remove_callback_t
dyntracer_get_environment_variable_remove_callback(dyntracer_t* dyntracer);
environment_variable_lookup_callback_t
dyntracer_get_environment_variable_lookup_callback(dyntracer_t* dyntracer);
environment_variable_exists_callback_t
dyntracer_get_environment_variable_exists_callback(dyntracer_t* dyntracer);
environment_context_sensitive_promise_eval_entry_callback_t
dyntracer_get_environment_context_sensitive_promise_eval_entry_callback(
    dyntracer_t* dyntracer);
environment_context_sensitive_promise_eval_exit_callback_t
dyntracer_get_environment_context_sensitive_promise_eval_exit_callback(
    dyntracer_t* dyntracer);

void dyntracer_set_dyntrace_entry_callback_t(dyntracer_t* dyntracer,
                                   dyntrace_entry_callback_t callback);

void dyntracer_set_dyntrace_exit_callback(dyntracer_t* dyntracer,
                                         dyntrace_exit_callback_t callback);
void dyntracer_set_deserialize_object_callback(
    dyntracer_t* dyntracer,
    deserialize_object_callback_t callback);
void dyntracer_set_closure_argument_list_creation_entry_callback(
    dyntracer_t* dyntracer,
    closure_argument_list_creation_entry_callback_t callback);
void dyntracer_set_closure_argument_list_creation_exit_callback(
    dyntracer_t* dyntracer,
    closure_argument_list_creation_exit_callback_t callback);
void dyntracer_set_closure_entry_callback(dyntracer_t* dyntracer,
                                         closure_entry_callback_t callback);
void dyntracer_set_closure_exit_callback(dyntracer_t* dyntracer,
                                        closure_exit_callback_t callback);
void dyntracer_set_builtin_entry_callback(dyntracer_t* dyntracer,
                                         builtin_entry_callback_t callback);
void dyntracer_set_builtin_exit_callback(dyntracer_t* dyntracer,
                                        builtin_exit_callback_t callback);
void dyntracer_set_special_entry_callback(dyntracer_t* dyntracer,
                                         special_entry_callback_t callback);
void dyntracer_set_special_exit_callback(dyntracer_t* dyntracer,
                                        special_exit_callback_t callback);
void dyntracer_set_substitute_call_callback(dyntracer_t* dyntracer,
                                           substitute_call_callback_t callback);
void dyntracer_set_assignment_call_callback(dyntracer_t* dyntracer,
                                           assignment_call_callback_t callback);
void dyntracer_set_promise_force_entry_callback(
    dyntracer_t* dyntracer,
    promise_force_entry_callback_t callback);
void dyntracer_set_promise_force_exit_callback(
    dyntracer_t* dyntracer,
    promise_force_exit_callback_t callback);
void dyntracer_set_promise_value_lookup_callback(
    dyntracer_t* dyntracer,
    promise_value_lookup_callback_t callback);
void dyntracer_set_promise_value_assign_callback(
    dyntracer_t* dyntracer,
    promise_value_assign_callback_t callback);
void dyntracer_set_promise_expression_lookup_callback(
    dyntracer_t* dyntracer,
    promise_expression_lookup_callback_t callback);
void dyntracer_set_promise_expression_assign_callback(
    dyntracer_t* dyntracer,
    promise_expression_assign_callback_t callback);
void dyntracer_set_promise_environment_lookup_callback(
    dyntracer_t* dyntracer,
    promise_environment_lookup_callback_t callback);
void dyntracer_set_promise_environment_assign_callback(
    dyntracer_t* dyntracer,
    promise_environment_assign_callback_t callback);
void dyntracer_set_promise_substitute_callback(
    dyntracer_t* dyntracer,
    promise_substitute_callback_t callback);
void dyntracer_set_eval_entry_callback(dyntracer_t* dyntracer,
                                      eval_entry_callback_t callback);
void dyntracer_set_eval_exit_callback(dyntracer_t* dyntracer,
                                     eval_exit_callback_t callback);
void dyntracer_set_gc_entry_callback(dyntracer_t* dyntracer,
                                    gc_entry_callback_t callback);
void dyntracer_set_gc_exit_callback(dyntracer_t* dyntracer,
                                   gc_exit_callback_t callback);
void dyntracer_set_gc_unmark_callback(dyntracer_t* dyntracer,
                                     gc_unmark_callback_t callback);
void dyntracer_set_gc_allocate_callback(dyntracer_t* dyntracer,
                                       gc_allocate_callback_t callback);
void dyntracer_set_context_entry_callback(dyntracer_t* dyntracer,
                                         context_entry_callback_t callback);
void dyntracer_set_context_exit_callback(dyntracer_t* dyntracer,
                                        context_exit_callback_t callback);
void dyntracer_set_context_jump_callback(dyntracer_t* dyntracer,
                                        context_jump_callback_t callback);
void dyntracer_set_S3_generic_entry_callback(
    dyntracer_t* dyntracer,
    S3_generic_entry_callback_t callback);
void dyntracer_set_S3_generic_exit_callback(dyntracer_t* dyntracer,
                                           S3_generic_exit_callback_t callback);
void dyntracer_set_S3_dispatch_entry_callback(
    dyntracer_t* dyntracer,
    S3_dispatch_entry_callback_t callback);
void dyntracer_set_S3_dispatch_exit_callback(
    dyntracer_t* dyntracer,
    S3_dispatch_exit_callback_t callback);
void dyntracer_set_S4_generic_entry_callback(
    dyntracer_t* dyntracer,
    S4_generic_entry_callback_t callback);
void dyntracer_set_S4_generic_exit_callback(dyntracer_t* dyntracer,
                                           S4_generic_exit_callback_t callback);
void dyntracer_set_S4_dispatch_argument_callback(
    dyntracer_t* dyntracer,
    S4_dispatch_argument_callback_t callback);
void dyntracer_set_environment_variable_define_callback(
    dyntracer_t* dyntracer,
    environment_variable_define_callback_t callback);
void dyntracer_set_environment_variable_assign_callback(
    dyntracer_t* dyntracer,
    environment_variable_assign_callback_t callback);
void dyntracer_set_environment_variable_remove_callback(
    dyntracer_t* dyntracer,
    environment_variable_remove_callback_t callback);
void dyntracer_set_environment_variable_lookup_callback(
    dyntracer_t* dyntracer,
    environment_variable_lookup_callback_t callback);
void dyntracer_set_environment_variable_exists_callback(
    dyntracer_t* dyntracer,
    environment_variable_exists_callback_t callback);
void dyntracer_set_environment_context_sensitive_promise_eval_entry_callback(
    dyntracer_t* dyntracer,
    environment_context_sensitive_promise_eval_entry_callback_t callback);
void dyntracer_set_environment_context_sensitive_promise_eval_exit_callback(
    dyntracer_t* dyntracer,
    environment_context_sensitive_promise_eval_exit_callback_t callback);

// ----------------------------------------------------------------------------
// STATE VARIABLES - For Internal Use Only
// ----------------------------------------------------------------------------

// the current dyntracer
extern dyntracer_t* dyntrace_active_dyntracer;
// name of currently executing callback
extern const char* dyntrace_active_dyntracer_callback_name;
// state of garbage collector before the hook is triggered
extern int dyntrace_garbage_collector_state;

    //SEXP do_dyntrace(SEXP call, SEXP op, SEXP args, SEXP rho);
SEXP dyntrace_trace_code(dyntracer_t* dyntracer, SEXP code, SEXP environment);
dyntracer_t* dyntrace_get_active_dyntracer();
int dyntrace_is_active();
int dyntrace_dyntracer_is_active();
// void dyntrace_disable_garbage_collector();
// void dyntrace_reinstate_garbage_collector();
// void dyntrace_enable_tracing();
// void dyntrace_reinstate_tracing();
// void dyntrace_disable_tracing();
// int dyntrace_is_tracing_enabled();
// int dyntrace_is_tracing_disabled();
SEXP dyntrace_lookup_environment(SEXP rho, SEXP key);
SEXP dyntrace_get_promise_expression(SEXP promise);
SEXP dyntrace_get_promise_environment(SEXP promise);
SEXP dyntrace_get_promise_value(SEXP promise);
int dyntrace_get_c_function_argument_evaluation(SEXP op);
int dyntrace_get_c_function_arity(SEXP op);
int dyntrace_get_primitive_offset(SEXP op);
const char* const dyntrace_get_c_function_name(SEXP op);
SEXP* dyntrace_get_symbol_table();

void(SET_PRENV_UNPROBED)(SEXP x, SEXP v);
void(SET_PRVALUE_UNPROBED)(SEXP x, SEXP v);
void(SET_PRCODE_UNPROBED)(SEXP x, SEXP v);

// ----------------------------------------------------------------------------
// helpers
// ----------------------------------------------------------------------------

SEXP serialize_sexp(SEXP s, int* linecount);
int findOp(void* addr);
int newhashpjw(const char* s);

/* Unwind the call stack in an orderly fashion */
/* calling the code installed by on.exit along the way */
/* and finally longjmping to the innermost TOPLEVEL context */
void NORET jump_to_top_ex(Rboolean, Rboolean, Rboolean, Rboolean, Rboolean);

#define dyntrace_log_error(error, ...)                                    \
    do {                                                                  \
        Rprintf("DYNTRACE LOG - ERROR - %s · %s(...) · %d - " error "\n", \
                __FILE__,                                                 \
                __func__,                                                 \
                __LINE__,                                                 \
                ##__VA_ARGS__);                                           \
        dyntrace_active_dyntracer_callback_name = NULL;                   \
        jump_to_top_ex(TRUE, TRUE, TRUE, TRUE, FALSE);                    \
    } while (0);

#define dyntrace_log_warning(warning, ...)                                \
    Rprintf("DYNTRACE LOG - WARNING - %s · %s(...) · %d - " warning "\n", \
            __FILE__,                                                     \
            __func__,                                                     \
            __LINE__,                                                     \
            ##__VA_ARGS__);

#define dyntrace_log_info(info, ...)                                \
    Rprintf("DYNTRACE LOG - INFO - %s · %s(...) · %d - " info "\n", \
            __FILE__,                                               \
            __func__,                                               \
            __LINE__,                                               \
            ##__VA_ARGS__);

#ifdef __cplusplus
}
#endif

#endif /* __DYNTRACE_H__ */
