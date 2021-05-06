#ifndef R_DYNTRACE_H
#define R_DYNTRACE_H

#include <Rinternals.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DYNTRACE_PROBE_HEADER(callback_name)                               \
    if (dyntrace_is_enabled() &&                                           \
        dyntrace_active_dyntracer != NULL &&                               \
        dyntrace_active_dyntracer->callback.callback_name != NULL) {       \
/*      if (dyntrace_active_dyntracer_callback_name != NULL) {    \
            dyntrace_log_error("[NESTED HOOK EXECUTION] - %s triggers %s", \
                               dyntrace_active_dyntracer_callback_name,    \
                               #callback_name);                            \
        }                                                                  \
        dyntrace_active_dyntracer_callback_name = #callback_name;
*/
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

#define DYNTRACE_PROBE_OBJECT_COERCE(input, output)         \
    DYNTRACE_PROBE_HEADER(object_coerce);                   \
    PROTECT(input);                                         \
    PROTECT(output);                                        \
    dyntrace_active_dyntracer->callback.object_coerce(      \
        dyntrace_active_dyntracer, input, output);          \
    UNPROTECT(2);                                           \
    DYNTRACE_PROBE_FOOTER(object_coerce);

#define DYNTRACE_PROBE_OBJECT_DUPLICATE(input, output, deep)  \
    DYNTRACE_PROBE_HEADER(object_duplicate);                  \
    PROTECT(input);                                           \
    PROTECT(output);                                          \
    dyntrace_active_dyntracer->callback.object_duplicate(     \
        dyntrace_active_dyntracer, input, output, deep);      \
    UNPROTECT(2);                                             \
    DYNTRACE_PROBE_FOOTER(object_duplicate);

#define DYNTRACE_PROBE_VECTOR_COPY(input, output)     \
    DYNTRACE_PROBE_HEADER(vector_copy);               \
    PROTECT(input);                                   \
    PROTECT(output);                                  \
    dyntrace_active_dyntracer->callback.vector_copy(  \
        dyntrace_active_dyntracer, input, output);    \
    UNPROTECT(2);                                     \
    DYNTRACE_PROBE_FOOTER(vector_copy);

#define DYNTRACE_PROBE_MATRIX_COPY(input, output)    \
    DYNTRACE_PROBE_HEADER(matrix_copy);              \
    PROTECT(input);                                  \
    PROTECT(output);                                 \
    dyntrace_active_dyntracer->callback.matrix_copy( \
        dyntrace_active_dyntracer, input, output);   \
    UNPROTECT(2);                                    \
    DYNTRACE_PROBE_FOOTER(matrix_copy);

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

#define DYNTRACE_PROBE_PROMISE_DELAYED_ASSIGN(name, promise, rho) \
    DYNTRACE_PROBE_HEADER(promise_delayed_assign);                \
    PROTECT(name);                                        \
    PROTECT(promise);                                     \
    PROTECT(rho);                                         \
    dyntrace_active_dyntracer->callback.promise_delayed_assign(   \
        dyntrace_active_dyntracer,                        \
        name,                                             \
        promise,                                          \
        rho);                                             \
    UNPROTECT(3);                                         \
    DYNTRACE_PROBE_FOOTER(promise_delayed_assign);

#define DYNTRACE_PROBE_PROMISE_LAZY_LOAD(name, promise, rho)      \
    DYNTRACE_PROBE_HEADER(promise_lazy_load);                     \
    PROTECT(name);                                        \
    PROTECT(promise);                                     \
    PROTECT(rho);                                         \
    dyntrace_active_dyntracer->callback.promise_lazy_load(        \
        dyntrace_active_dyntracer,                        \
        name,                                             \
        promise,                                          \
        rho);                                             \
    UNPROTECT(3);                                         \
    DYNTRACE_PROBE_FOOTER(promise_lazy_load);

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
    dyntrace_active_dyntracer->callback.gc_unmark(dyntrace_active_dyntracer, \
                                                  object);                   \
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

#define DYNTRACE_PROBE_CALL_HANDLER_ENTRY(context, expr, env) \
    DYNTRACE_PROBE_HEADER(call_handler_entry);                \
    PROTECT(expr);                                            \
    PROTECT(env);                                             \
    dyntrace_active_dyntracer->callback.call_handler_entry(   \
        dyntrace_active_dyntracer, context, expr, env);       \
        UNPROTECT(2);                                         \
    DYNTRACE_PROBE_FOOTER(call_handler_entry);

#define DYNTRACE_PROBE_CALL_HANDLER_EXIT(context, expr, env) \
    DYNTRACE_PROBE_HEADER(call_handler_exit);                \
    PROTECT(expr);                                           \
    PROTECT(env);                                            \
    dyntrace_active_dyntracer->callback.call_handler_exit(   \
        dyntrace_active_dyntracer, context, expr, env);      \
    UNPROTECT(2);                                            \
    DYNTRACE_PROBE_FOOTER(call_handler_exit);

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

#define DYNTRACE_PROBE_ENVIRONMENT_FUNCTION_CONTEXT_LOOKUP(                  \
    symbol, value, rho)                                                      \
    DYNTRACE_PROBE_HEADER(environment_function_context_lookup);              \
    PROTECT(symbol);                                                         \
    PROTECT(value);                                                          \
    PROTECT(rho);                                                            \
    dyntrace_active_dyntracer->callback                                      \
    .environment_function_context_lookup(                                    \
            dyntrace_active_dyntracer, symbol, value, rho);                  \
    UNPROTECT(3);                                                            \
    DYNTRACE_PROBE_FOOTER(environment_function_context_lookup);

#define DYNTRACE_PROBE_ERROR(call, format, ap)                \
    DYNTRACE_PROBE_HEADER(error);                             \
    dyntrace_active_dyntracer->callback                       \
    .error(dyntrace_active_dyntracer, call, format, ap);      \
    DYNTRACE_PROBE_FOOTER(error);

#define DYNTRACE_PROBE_ATTRIBUTE_SET(object, name, value)            \
    DYNTRACE_PROBE_HEADER(attribute_set);                            \
    PROTECT(object);                                                 \
    PROTECT(name);                                                   \
    PROTECT(value);                                                  \
    dyntrace_active_dyntracer->callback                              \
    .attribute_set(dyntrace_active_dyntracer, object, name, value);  \
    UNPROTECT(3);                                                    \
    DYNTRACE_PROBE_FOOTER(attribute_set);

/* ----------------------------------------------------------------------------
   DYNTRACE TYPE DEFINITIONS
---------------------------------------------------------------------------- */

struct dyntrace_result_t {
    int error_code;
    SEXP value;
};

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

#define DYNTRACE_CALLBACK_MAP(MACRO)                                           \
    MACRO(dyntrace_entry,                                                      \
          dyntracer_t* dyntracer,                                              \
          SEXP expression,                                                     \
          SEXP environment)                                                    \
    MACRO(dyntrace_exit,                                                       \
          dyntracer_t* dyntracer,                                              \
          SEXP expression,                                                     \
          SEXP environment,                                                    \
          SEXP result,                                                         \
          int error)                                                           \
    MACRO(deserialize_object, dyntracer_t* dyntracer, SEXP object)             \
    MACRO(object_coerce,                                                       \
          dyntracer_t* dyntracer,                                              \
          SEXP input,                                                          \
          SEXP output)                                                         \
    MACRO(object_duplicate,                                                    \
          dyntracer_t* dyntracer,                                              \
          SEXP input,                                                          \
          SEXP output,                                                         \
          int deep)                                                            \
    MACRO(vector_copy,                                                         \
          dyntracer_t* dyntracer,                                              \
          SEXP input,                                                          \
          SEXP output)                                                         \
    MACRO(matrix_copy,                                                         \
          dyntracer_t* dyntracer,                                              \
          SEXP input,                                                          \
          SEXP output)                                                         \
    MACRO(closure_argument_list_creation_entry,                                \
          dyntracer_t* dyntracer,                                              \
          SEXP formals,                                                        \
          SEXP actuals,                                                        \
          SEXP parent_rho)                                                     \
    MACRO(closure_argument_list_creation_exit,                                 \
          dyntracer_t* dyntracer,                                              \
          SEXP rho)                                                            \
    MACRO(closure_entry,                                                       \
          dyntracer_t* dyntracer,                                              \
          const SEXP call,                                                     \
          const SEXP op,                                                       \
          const SEXP args,                                                     \
          const SEXP rho,                                                      \
          dyntrace_dispatch_t dispatch)                                        \
    MACRO(closure_exit,                                                        \
          dyntracer_t* dyntracer,                                              \
          const SEXP call,                                                     \
          const SEXP op,                                                       \
          const SEXP args,                                                     \
          const SEXP rho,                                                      \
          dyntrace_dispatch_t dispatch,                                        \
          const SEXP return_value)                                             \
    MACRO(builtin_entry,                                                       \
          dyntracer_t* dyntracer,                                              \
          const SEXP call,                                                     \
          const SEXP op,                                                       \
          const SEXP args,                                                     \
          const SEXP rho,                                                      \
          dyntrace_dispatch_t dispatch)                                        \
    MACRO(builtin_exit,                                                        \
          dyntracer_t* dyntracer,                                              \
          const SEXP call,                                                     \
          const SEXP op,                                                       \
          const SEXP args,                                                     \
          const SEXP rho,                                                      \
          dyntrace_dispatch_t dispatch,                                        \
          const SEXP return_value)                                             \
    MACRO(special_entry,                                                       \
          dyntracer_t* dyntracer,                                              \
          const SEXP call,                                                     \
          const SEXP op,                                                       \
          const SEXP args,                                                     \
          const SEXP rho,                                                      \
          dyntrace_dispatch_t dispatch)                                        \
    MACRO(special_exit,                                                        \
          dyntracer_t* dyntracer,                                              \
          const SEXP call,                                                     \
          const SEXP op,                                                       \
          const SEXP args,                                                     \
          const SEXP rho,                                                      \
          dyntrace_dispatch_t dispatch,                                        \
          const SEXP return_value)                                             \
    MACRO(substitute_call,                                                     \
          dyntracer_t* dyntracer,                                              \
          const SEXP expression,                                               \
          const SEXP environment,                                              \
          const SEXP rho,                                                      \
          const SEXP return_value)                                             \
    MACRO(promise_delayed_assign,                                              \
          dyntracer_t* dyntracer,                                              \
          const SEXP name,                                                     \
          const SEXP promise,                                                  \
          const SEXP rho)                                                      \
    MACRO(promise_lazy_load,                                                   \
          dyntracer_t* dyntracer,                                              \
          const SEXP name,                                                     \
          const SEXP promise,                                                  \
          const SEXP rho)                                                      \
    MACRO(assignment_call,                                                     \
          dyntracer_t* dyntracer,                                              \
          const SEXP call,                                                     \
          const SEXP op,                                                       \
          dyntrace_assignment_t assignment_type,                               \
          const SEXP lhs,                                                      \
          const SEXP rhs,                                                      \
          const SEXP assign_env,                                               \
          const SEXP eval_env)                                                 \
    MACRO(promise_force_entry, dyntracer_t* dyntracer, const SEXP promise)     \
    MACRO(promise_force_exit, dyntracer_t* dyntracer, const SEXP promise)      \
    MACRO(promise_value_lookup, dyntracer_t* dyntracer, const SEXP promise)    \
    MACRO(promise_value_assign,                                                \
          dyntracer_t* dyntracer,                                              \
          const SEXP promise,                                                  \
          const SEXP value)                                                    \
    MACRO(                                                                     \
        promise_expression_lookup, dyntracer_t* dyntracer, const SEXP promise) \
    MACRO(promise_expression_assign,                                           \
          dyntracer_t* dyntracer,                                              \
          const SEXP promise,                                                  \
          const SEXP expression)                                               \
    MACRO(promise_environment_lookup,                                          \
          dyntracer_t* dyntracer,                                              \
          const SEXP promise)                                                  \
    MACRO(promise_environment_assign,                                          \
          dyntracer_t* dyntracer,                                              \
          const SEXP promise,                                                  \
          const SEXP environment)                                              \
    MACRO(promise_substitute, dyntracer_t* dyntracer, const SEXP promise)      \
    MACRO(eval_entry,                                                          \
          dyntracer_t* dyntracer,                                              \
          const SEXP expression,                                               \
          const SEXP rho)                                                      \
    MACRO(eval_exit,                                                           \
          dyntracer_t* dyntracer,                                              \
          const SEXP expression,                                               \
          const SEXP rho,                                                      \
          SEXP return_value)                                                   \
    MACRO(gc_entry, dyntracer_t* dyntracer, const size_t size_needed)          \
    MACRO(gc_exit, dyntracer_t* dyntracer, int gc_count)                       \
    MACRO(gc_unmark, dyntracer_t* dyntracer, const SEXP object)                \
    MACRO(gc_allocate, dyntracer_t* dyntracer, const SEXP object)              \
    MACRO(context_entry, dyntracer_t* dyntracer, void* context)                \
    MACRO(context_exit, dyntracer_t* dyntracer, void* context)                 \
    MACRO(context_jump,                                                        \
          dyntracer_t* dyntracer,                                              \
          void* context,                                                       \
          const SEXP return_value,                                             \
          int restart)                                                         \
    MACRO(call_handler_entry, dyntracer_t* dyntracer, void* context, SEXP r_expression, SEXP r_environment) \
    MACRO(call_handler_exit, dyntracer_t* dyntracer, void* context, SEXP r_expression, SEXP r_environment)  \
    MACRO(S3_generic_entry,                                                    \
          dyntracer_t* dyntracer,                                              \
          const char* generic,                                                 \
          const SEXP generic_method,                                           \
          const SEXP object)                                                   \
    MACRO(S3_generic_exit,                                                     \
          dyntracer_t* dyntracer,                                              \
          const char* generic,                                                 \
          const SEXP generic_method,                                           \
          const SEXP object,                                                   \
          const SEXP retval)                                                   \
    MACRO(S3_dispatch_entry,                                                   \
          dyntracer_t* dyntracer,                                              \
          const char* generic,                                                 \
          const SEXP cls,                                                      \
          const SEXP generic_method,                                           \
          const SEXP specific_method,                                          \
          const SEXP objects)                                                  \
    MACRO(S3_dispatch_exit,                                                    \
          dyntracer_t* dyntracer,                                              \
          const char* generic,                                                 \
          const SEXP cls,                                                      \
          const SEXP generic_method,                                           \
          const SEXP specific_method,                                          \
          const SEXP return_value,                                             \
          const SEXP objects)                                                  \
    MACRO(S4_generic_entry,                                                    \
          dyntracer_t* dyntracer,                                              \
          const SEXP fname,                                                    \
          const SEXP env,                                                      \
          const SEXP fdef)                                                     \
    MACRO(S4_generic_exit,                                                     \
          dyntracer_t* dyntracer,                                              \
          const SEXP fname,                                                    \
          const SEXP env,                                                      \
          const SEXP fdef,                                                     \
          const SEXP return_value)                                             \
    MACRO(S4_dispatch_argument, dyntracer_t* dyntracer, const SEXP argument)   \
    MACRO(environment_variable_define,                                         \
          dyntracer_t* dyntracer,                                              \
          const SEXP symbol,                                                   \
          const SEXP value,                                                    \
          const SEXP rho)                                                      \
    MACRO(environment_variable_assign,                                         \
          dyntracer_t* dyntracer,                                              \
          const SEXP symbol,                                                   \
          const SEXP value,                                                    \
          const SEXP rho)                                                      \
    MACRO(environment_variable_remove,                                         \
          dyntracer_t* dyntracer,                                              \
          const SEXP symbol,                                                   \
          const SEXP rho)                                                      \
    MACRO(environment_variable_lookup,                                         \
          dyntracer_t* dyntracer,                                              \
          const SEXP symbol,                                                   \
          const SEXP value,                                                    \
          const SEXP rho)                                                      \
    MACRO(environment_variable_exists,                                         \
          dyntracer_t* dyntracer,                                              \
          const SEXP symbol,                                                   \
          SEXP rho)                                                            \
    MACRO(environment_function_context_lookup,                                 \
          dyntracer_t* dyntracer,                                              \
          const SEXP symbol,                                                   \
          SEXP promise,                                                        \
          SEXP rho)                                                            \
    MACRO(error,                                                               \
          dyntracer_t* dyntracer,                                              \
          SEXP call,                                                           \
          const char* format,                                                  \
          va_list ap)                                                          \
    MACRO(attribute_set,                                                       \
          dyntracer_t* dyntracer,                                              \
          SEXP object,                                                         \
          SEXP name,                                                           \
          SEXP value)


#define DYNTRACE_CALLBACK_TYPEDEF(NAME, ...) typedef void (*NAME##_callback_t)(__VA_ARGS__);

DYNTRACE_CALLBACK_MAP(DYNTRACE_CALLBACK_TYPEDEF)

#undef DYNTRACE_CALLBACK_TYPEDEF


struct dyntracer_callback_t {

#define DYNTRACE_CALLBACK_FIELD(NAME, ...) NAME##_callback_t NAME;

DYNTRACE_CALLBACK_MAP(DYNTRACE_CALLBACK_FIELD)

#undef DYNTRACE_CALLBACK_FIELD
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
void dyntracer_remove_data(dyntracer_t* dyntracer);


#define DYNTRACE_CALLBACK_API(NAME, ...)                                                      \
    int dyntracer_has_##NAME##_callback(dyntracer_t* dyntracer);                              \
    NAME##_callback_t dyntracer_get_##NAME##_callback(dyntracer_t* dyntracer);                \
    void dyntracer_set_##NAME##_callback(dyntracer_t* dyntracer, NAME##_callback_t callback); \
    void dyntracer_remove_##NAME##_callback(dyntracer_t* dyntracer);

DYNTRACE_CALLBACK_MAP(DYNTRACE_CALLBACK_API)

#undef DYNTRACE_CALLBACK_API

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
struct dyntrace_result_t dyntrace_trace_code(dyntracer_t* dyntracer, SEXP code, SEXP environment);
dyntracer_t* dyntrace_get_active_dyntracer();
int dyntrace_is_active();
int dyntrace_dyntracer_is_active();
SEXP dyntrace_lookup_environment(SEXP rho, SEXP key);
SEXP dyntrace_get_promise_expression(SEXP promise);
SEXP dyntrace_get_promise_environment(SEXP promise);
SEXP dyntrace_get_promise_value(SEXP promise);
void* dyntrace_get_funtab();
int dyntrace_get_c_function_argument_evaluation(SEXP op);
int dyntrace_get_c_function_arity(SEXP op);
int dyntrace_get_primitive_offset(SEXP op);
const char* const dyntrace_get_c_function_name(SEXP op);
SEXP* dyntrace_get_symbol_table();
int dyntrace_get_frame_depth();

SEXP dyntrace_context_get_promargs(void *context);
SEXP dyntrace_context_get_cloenv(void *context);
SEXP dyntrace_context_get_callfun(void *context);
SEXP dyntrace_context_get_call(void *context);
SEXP dyntrace_get_replace_funs_table();
SEXP dyntrace_get_s4_extends_table();

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

#endif /* R_DYNTRACE_H */
