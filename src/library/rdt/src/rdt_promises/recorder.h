//
// Created by nohajc on 28.3.17.
//

#ifndef R_3_3_1_RECORDER_H
#define R_3_3_1_RECORDER_H

#include <tuple>
#include "tuple_for_each.h"

#include "../rdt.h"
#include "tracer_sexpinfo.h"
#include "tracer_state.h"

template<typename Impl>
class recorder_t {
private:
    Impl& impl() {
        return *static_cast<Impl*>(this);
    }

public:
    call_info_t function_entry_get_info(const SEXP call, const SEXP op, const SEXP rho) {
        call_info_t info;

        const char *name = get_name(call);
        const char *ns = get_ns_name(op);

        info.type = is_byte_compiled(call) ? "=> bcod" : "=> func";
        info.call_type = is_byte_compiled(call) ? 1 : 0;
        info.fn_id = get_function_id(op);
        info.call_ptr = get_sexp_address(rho);
#ifdef RDT_CALL_ID
        info.call_id = make_funcall_id(op);
#else
        info.call_id = make_funcall_id(rho);
#endif
        char *location = get_location(op);
        if (location != NULL)
            info.loc = location;
        free(location);

        if (ns) {
            info.fqfn = string(ns) + "::" + CHKSTR(name);
        } else {
            if (name != NULL)
                info.fqfn = name;
        }

        info.arguments = get_arguments(op, rho);
        info.fn_definition = get_expression(op);

        return info;
    }

    // TODO: merge duplicate code from function_entry/exit
    call_info_t function_exit_get_info(const SEXP call, const SEXP op, const SEXP rho) {
        call_info_t info;

        const char *name = get_name(call);
        const char *ns = get_ns_name(op);

        info.type = is_byte_compiled(call) ? "<= bcod" : "<= func"; // FIXME this seems... inelegant now
        info.fn_id = get_function_id(op);
        info.call_id = STATE(fun_stack).top();

        char *location = get_location(op);
        if (location != NULL)
            info.loc = location;
        free(location);

        if (ns) {
            info.fqfn = string(ns) + "::" + CHKSTR(name);
        } else {
            if (name != NULL)
                info.fqfn = name;
        }

        info.arguments = get_arguments(op, rho);

        return info;
    }

    call_info_t builtin_entry_get_info(const SEXP call, const SEXP op, const SEXP rho) {
        call_info_t info;

        const char *name = get_name(call);
        if (name != NULL)
            info.name = name;
        info.fn_id = get_function_id(op);
        info.fqfn = info.name;
        info.call_type = 2;

        info.call_ptr = get_sexp_address(rho);
#ifdef RDT_CALL_ID
        info.call_id = make_funcall_id(op);
#else
        // Builtins have no environment of their own
        // we take the parent env rho and add 1 to it to create a new pseudo-address
        // it will be unique because real pointers are aligned (no odd addresses)
        info.call_id = make_funcall_id(rho) | 1;
#endif

        return info;
    }

    call_info_t builtin_exit_get_info(const SEXP call, const SEXP op, const SEXP rho) {
        call_info_t info;

        const char *name = get_name(call);
        if (name != NULL)
            info.name = name;
        info.fn_id = get_function_id(op);
        info.call_id = STATE(fun_stack).top();
        if (name != NULL)
            info.fqfn = name;
        info.call_type = 2;

        return info;
    }

//    prom_id_t promise_created_get_info(const SEXP prom) {
//
//    }

private:
    prom_info_t promise_get_info(const SEXP symbol, const SEXP rho) {
        prom_info_t info;

        const char *name = get_name(symbol);
        if (name != NULL)
            info.name = name;

        SEXP promise_expression = get_promise(symbol, rho);
        info.prom_id = get_promise_id(promise_expression);
        info.in_call_id = STATE(fun_stack).top();
        info.from_call_id = STATE(promise_origin)[info.prom_id];

        return info;
    }

public:
    prom_info_t force_promise_entry_get_info(const SEXP symbol, const SEXP rho) {
        return promise_get_info(symbol, rho);
    }

    prom_info_t force_promise_exit_get_info(const SEXP symbol, const SEXP rho) {
        return promise_get_info(symbol, rho);
    }

    prom_info_t promise_lookup_get_info(const SEXP symbol, const SEXP rho) {
        return promise_get_info(symbol, rho);
    }

#define DELEGATE2(func, info_struct) \
    void func##_process(const info_struct & info) { \
        impl().func(info); \
    }

#define DELEGATE1(func) \
    void func##_process() { \
        impl().func(); \
    }

    // Macro overloading trick: http://stackoverflow.com/a/11763277/6846474
#define GET_MACRO(_1, _2, NAME, ...) NAME
#define DELEGATE(...) GET_MACRO(__VA_ARGS__, DELEGATE2, DELEGATE1)(__VA_ARGS__)

    DELEGATE(function_entry, call_info_t)
    DELEGATE(function_exit, call_info_t)
    DELEGATE(builtin_entry, call_info_t)
    DELEGATE(builtin_exit, call_info_t)
    DELEGATE(force_promise_entry, prom_info_t)
    DELEGATE(force_promise_exit, prom_info_t)
    DELEGATE(promise_created, prom_id_t)
    DELEGATE(promise_lookup, prom_info_t)

    DELEGATE(init_recorder)
    DELEGATE(start_trace)
    DELEGATE(finish_trace)
    DELEGATE(unwind, vector<call_id_t>)

#undef DELEGATE
#undef DELEGATE1
#undef DELEGATE2
};


template<typename ...Rec>
class compose : public recorder_t<compose<Rec...>> {
    std::tuple<Rec...> rec;

public:
#define COMPOSE2(func, info_struct) \
    void func(const info_struct & info) { \
        tuple_for_each(rec, [&info](auto & r) { \
            r.func(info); \
        }); \
    }

#define COMPOSE1(func) \
    void func() { \
        tuple_for_each(rec, [](auto & r) { \
            r.func(); \
        }); \
    }

#define COMPOSE(...) GET_MACRO(__VA_ARGS__, COMPOSE2, COMPOSE1)(__VA_ARGS__)

    COMPOSE(function_entry, call_info_t)
    COMPOSE(function_exit, call_info_t)
    COMPOSE(builtin_entry, call_info_t)
    COMPOSE(builtin_exit, call_info_t)
    COMPOSE(force_promise_entry, prom_info_t)
    COMPOSE(force_promise_exit, prom_info_t)
    COMPOSE(promise_created, prom_id_t)
    COMPOSE(promise_lookup, prom_info_t)

    COMPOSE(init_recorder)
    COMPOSE(start_trace)
    COMPOSE(finish_trace)
    COMPOSE(unwind, vector<call_id_t>)

#undef COMPOSE
#undef COMPOSE1
#undef COMPOSE2
#undef GET_MACRO
};

#endif //R_3_3_1_RECORDER_H
