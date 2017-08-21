//
// Created by nohajc on 28.3.17.
//

#ifndef R_3_3_1_RECORDER_H
#define R_3_3_1_RECORDER_H

#include "tuple_for_each.h"
#include <inspect.h>
#include <tuple>
#include <chrono>
#include <ctime>

//#include <Defn.h> // We need this for R_Funtab
#include "tracer_sexpinfo.h"
#include "tracer_state.h"
#include <rdt.h>
#include <set>

template<typename Impl>
class recorder_t {
protected:
    Impl& impl() {
        return *static_cast<Impl*>(this);
    }

private:
    void get_metadatum(metadata_t & metadata, string key) {
        char * value = getenv(key.c_str());
        if (value == NULL)
            metadata[key] = "";
        else
            metadata[key] = string(value);
    }

public:
    void get_environment_metadata(metadata_t & metadata) {
        get_metadatum(metadata, "RDT_COMPILE_VIGNETTE");
        get_metadatum(metadata, "R_COMPILE_PKGS");
        get_metadatum(metadata, "R_DISABLE_BYTECODE");
        get_metadatum(metadata, "R_ENABLE_JIT");
        get_metadatum(metadata, "R_KEEP_PKG_SOURCE");
        get_metadatum(metadata, "R_ENABLE_JIT");
    }

    void get_current_time_metadata(metadata_t & metadata, string prefix) {
        chrono::time_point<chrono::system_clock> time_point = chrono::system_clock::now();
        time_t time = chrono::system_clock::to_time_t(time_point);
        string kludge = ctime(&time);
        metadata["RDT_TRACE_" + prefix + "_DATE"] = kludge.substr(0, kludge.length() - 1);
        metadata["RDT_TRACE_" + prefix + "_TIME"] = to_string(static_cast<long int>(time));
    }

    closure_info_t function_entry_get_info(const SEXP call, const SEXP op, const SEXP rho) {
        closure_info_t info;

        const char *name = get_name(call);
        const char *ns = get_ns_name(op);

        info.fn_compiled = is_byte_compiled(op);
        info.fn_type = function_type::CLOSURE;
        info.fn_id = get_function_id(op);
        info.fn_addr = get_function_addr(op);
        info.call_ptr = get_sexp_address(rho);
        info.call_id = make_funcall_id(op);
        //info.call_id = make_funcall_id(rho);


        call_stack_elem_t elem = STATE(fun_stack).back();
        info.parent_call_id = get<0>(elem);

        char *location = get_location(op);
        if (location != NULL)
            info.loc = location;
        free(location);

        char *callsite = get_callsite(1);
        if (callsite != NULL)
            info.callsite = callsite;
        free(callsite);

        if (ns) {
            info.name = string(ns) + "::" + CHKSTR(name);
        } else {
            if (name != NULL)
                info.name = name;
        }

        info.arguments = get_arguments(info.call_id, op, rho);
        info.fn_definition = get_expression(op);

        info.recursion = is_recursive(info.fn_id);

        get_stack_parent(info, STATE(full_stack));
        info.in_prom_id = get_parent_promise();

        stack_event_t stack_elem;
        stack_elem.type = stack_type::CALL;
        stack_elem.call_id = info.call_id;
        STATE(full_stack).push_back(stack_elem);

        return info;
    }

    closure_info_t function_exit_get_info(const SEXP call, const SEXP op, const SEXP rho) {
        closure_info_t info;

        const char *name = get_name(call);
        const char *ns = get_ns_name(op);

        info.fn_compiled = is_byte_compiled(op);
        info.fn_id = get_function_id(op);
        info.fn_addr = get_function_addr(op);
        call_stack_elem_t elem = STATE(fun_stack).back();
        info.call_id = get<0>(elem);
        info.fn_type = function_type::CLOSURE;

        char *location = get_location(op);
        if (location != NULL)
            info.loc = location;
        free(location);

        char *callsite = get_callsite(0);
        if (callsite != NULL)
            info.callsite = callsite;
        free(callsite);

        if (ns) {
            info.name = string(ns) + "::" + CHKSTR(name);
        } else {
            if (name != NULL)
                info.name = name;
        }

        info.arguments = get_arguments(info.call_id, op, rho);
        info.fn_definition = get_expression(op);

        STATE(fun_stack).pop_back();
        call_stack_elem_t elem_parent = STATE(fun_stack).back();
        info.parent_call_id = get<0>(elem_parent);

        info.recursion = is_recursive(info.fn_id);

        STATE(full_stack).pop_back();
        get_stack_parent(info, STATE(full_stack));
        info.in_prom_id = get_parent_promise();

        return info;
    }

    builtin_info_t builtin_entry_get_info(const SEXP call, const SEXP op, const SEXP rho, function_type fn_type) {
        builtin_info_t info;

        const char *name = get_name(call);
        if (name != NULL)
            info.name = name;
        info.fn_id = get_function_id(op);
        info.fn_addr = get_function_addr(op);
        info.name = info.name;
        info.fn_type = fn_type;
        info.fn_compiled = is_byte_compiled(op);
        info.fn_definition = get_expression(op);

        //R_FunTab[PRIMOFFSET(op)].eval % 100 )/10 ==

        call_stack_elem_t elem = STATE(fun_stack).back();
        info.parent_call_id = get<0>(elem);

        char *location = get_location(op);
        if (location != NULL) {
            info.loc = location;
        }
        free(location);

        char *callsite = get_callsite(0);
        if (callsite != NULL)
            info.callsite = callsite;
        free(callsite);

        info.call_ptr = get_sexp_address(rho);
        info.call_id = make_funcall_id(op);

        // XXX This is a remnant of an RDT_CALL_ID ifdef
        // Builtins have no environment of their own
        // we take the parent env rho and add 1 to it to create a new pseudo-address
        // it will be unique because real pointers are aligned (no odd addresses)
        // info.call_id = make_funcall_id(rho) | 1;

        info.recursion = is_recursive(info.fn_id);

        get_stack_parent(info, STATE(full_stack));
        info.in_prom_id = get_parent_promise();

        stack_event_t stack_elem;
        stack_elem.type = stack_type::CALL;
        stack_elem.call_id = info.call_id;
        STATE(full_stack).push_back(stack_elem);

        return info;
    }

    builtin_info_t builtin_exit_get_info(const SEXP call, const SEXP op, const SEXP rho, function_type fn_type) {
        builtin_info_t info;

        const char *name = get_name(call);
        if (name != NULL)
            info.name = name;
        info.fn_id = get_function_id(op);
        info.fn_addr = get_function_addr(op);
        call_stack_elem_t elem = STATE(fun_stack).back();
        info.call_id = get<0>(elem);
        if (name != NULL)
            info.name = name;
        info.fn_type = fn_type;
        info.fn_compiled = is_byte_compiled(op);
        info.fn_definition = get_expression(op);

        call_stack_elem_t parent_elem = STATE(fun_stack).back();
        info.parent_call_id = get<0>(parent_elem);
        info.recursion = is_recursive(info.fn_id);

        char *location = get_location(op);
        if (location != NULL)
            info.loc = location;
        free(location);

        char *callsite = get_callsite(0);
        if (callsite != NULL)
            info.callsite = callsite;
        free(callsite);

        STATE(full_stack).pop_back();
        get_stack_parent(info, STATE(full_stack));
        info.in_prom_id = get_parent_promise();

        return info;
    }

    gc_info_t gc_exit_get_info(int gc_count, double vcells, double ncells) {
        gc_info_t info;
        info.vcells = vcells;
        info.ncells = ncells;
        info.counter = gc_count;
        return info;
    }

private:
    recursion_type is_recursive(fn_id_t function) {
        for (vector<call_stack_elem_t>::reverse_iterator i = STATE(fun_stack).rbegin(); i != STATE(fun_stack).rend(); ++i) {
            call_id_t cursor_call = get<0>(*i);
            fn_id_t cursor_function = get<1>(*i);
            function_type cursor_type = get<2>(*i);

            if (cursor_call == 0) {
                // end of stack
                return recursion_type::UNKNOWN;
            }

            if (cursor_function == function) {
                return recursion_type::RECURSIVE;
            }

            if (cursor_type == function_type::BUILTIN || cursor_type == function_type::CLOSURE) {
                return recursion_type::NOT_RECURSIVE;
            }

            // inside a different function, but one that doesn't matter, recursion still possible
        }
    }

    void set_distances_and_lifestyle(prom_info_t & info) {
        if (info.in_call_id == info.from_call_id) {
            info.lifestyle = lifestyle_type::LOCAL;
            info.effective_distance_from_origin = 0;
            info.actual_distance_from_origin = 0;
        } else {
            auto lifestyle_info = judge_promise_lifestyle(info.from_call_id);
            info.lifestyle = get<0>(lifestyle_info);
            info.effective_distance_from_origin = get<1>(lifestyle_info);
            info.actual_distance_from_origin = get<2>(lifestyle_info);
        }
    }

    tuple<lifestyle_type, int, int> judge_promise_lifestyle(call_id_t from_call_id) {
        int effective_distance = 0;
        int actual_distance = 0;
        for (vector<call_stack_elem_t>::reverse_iterator i = STATE(fun_stack).rbegin(); i != STATE(fun_stack).rend(); ++i) {
            call_id_t cursor = get<0>(*i);
            function_type type = get<2>(*i);

            if (cursor == from_call_id)
                if (effective_distance == 0) {
                    if (actual_distance == 0){
                        return tuple<lifestyle_type, int, int>(lifestyle_type::IMMEDIATE_LOCAL, effective_distance, actual_distance);
                    } else {
                        return tuple<lifestyle_type, int, int>(lifestyle_type::LOCAL, effective_distance, actual_distance);
                    }
                } else {
                    if (effective_distance == 1) {
                        return tuple<lifestyle_type, int, int>(lifestyle_type::IMMEDIATE_BRANCH_LOCAL, effective_distance, actual_distance);
                    } else {
                        return tuple<lifestyle_type, int, int>(lifestyle_type::BRANCH_LOCAL, effective_distance, actual_distance);
                    }
                }

            if (cursor == 0) {
                return tuple<lifestyle_type, int, int>(lifestyle_type::ESCAPED, -1, -1); // reached root, parent must be in a different branch--promise escaped
            }

            actual_distance++;
            if (type == function_type::BUILTIN || type == function_type::CLOSURE) {
                effective_distance++;
            }
        }
    }

    inline void get_full_type(SEXP promise, SEXP rho, full_sexp_type & result) { // FIXME remove rho
        set<SEXP> visited;
        get_full_type_inner(PRCODE(promise), PRENV(promise), result, visited);
    }

    void get_full_type_inner(SEXP sexp, SEXP rho, full_sexp_type & result, set<SEXP> & visited) {
        sexp_type type = static_cast<sexp_type>(TYPEOF(sexp));
        result.push_back(type);

        if (visited.find(sexp) != visited.end()) {
            result.push_back(sexp_type::OMEGA);
            return;
        } else {
            visited.insert(sexp);
        }

        if (type == sexp_type::PROM) {
            get_full_type_inner(PRCODE(sexp), PRENV(sexp), result, visited);
            return;
        }

        // Question... are all BCODEs functions?
        if (type == sexp_type::BCODE) {
//            bool try_to_attach_symbol_value = (rho != R_NilValue) ? isEnvironment(rho) : false;
//            if (!try_to_attach_symbol_value) return;
//
//            SEXP uncompiled_sexp = BODY_EXPR(sexp);
//            SEXP underlying_expression = findVar(PRCODE(uncompiled_sexp), rho);
//
//            if (underlying_expression == R_UnboundValue) return;
//            if (underlying_expression == R_MissingArg) return;
//
//            PROTECT(underlying_expression);
//            //get_full_type(underlying_expression, rho, result, visited);
//           UNPROTECT(1);

            //Rprintf("hi from dbg\n`");
            return;

        }

        if (type == sexp_type::SYM) {
            bool try_to_attach_symbol_value = (rho != R_NilValue) ? isEnvironment(rho) : false;
            if (!try_to_attach_symbol_value) return;

            SEXP symbol_points_to = findVar(sexp, rho);

            if (symbol_points_to == R_UnboundValue) return;
            if (symbol_points_to == R_MissingArg) return;
            //if (TYPEOF(symbol_points_to) == SYMSXP) return;

            get_full_type_inner(symbol_points_to, rho, result, visited);

            return;
        }
    }

public:
    prom_basic_info_t create_promise_get_info(const SEXP promise, const SEXP rho) {
        prom_basic_info_t info;

        info.prom_id = make_promise_id(promise);
        STATE(fresh_promises).insert(info.prom_id);

        info.prom_type = static_cast<sexp_type>(TYPEOF(PRCODE(promise)));
        get_full_type(promise, rho, info.full_type);

        get_stack_parent(info, STATE(full_stack));
        info.in_prom_id = get_parent_promise();
        info.depth = get_no_of_ancestor_promises_on_stack();

        return info;
    }

    prom_info_t force_promise_entry_get_info(const SEXP symbol, const SEXP rho) {
        prom_info_t info;

        const char *name = get_name(symbol);
        if (name != NULL)
            info.name = name;

        SEXP promise_expression = get_promise(symbol, rho);
        info.prom_id = get_promise_id(promise_expression);

        call_stack_elem_t call_stack_elem = STATE(fun_stack).back();
        info.in_call_id = get<0>(call_stack_elem);
        info.from_call_id = STATE(promise_origin)[info.prom_id];

        set_distances_and_lifestyle(info);

        info.prom_type = static_cast<sexp_type>(TYPEOF(PRCODE(promise_expression)));
        get_full_type(promise_expression, rho, info.full_type);
        info.return_type = sexp_type::OMEGA;

        get_stack_parent(info, STATE(full_stack));
        info.in_prom_id = get_parent_promise();
        info.depth = get_no_of_ancestor_promises_on_stack();

        stack_event_t stack_elem;
        stack_elem.type = stack_type::PROMISE;
        stack_elem.promise_id = info.prom_id;
        STATE(full_stack).push_back(stack_elem);

        return info;
    }

    prom_info_t force_promise_exit_get_info(const SEXP symbol, const SEXP rho, const SEXP val) {
        prom_info_t info;

        const char *name = get_name(symbol);
        if (name != NULL)
            info.name = name;

        SEXP promise_expression = get_promise(symbol, rho);
        info.prom_id = get_promise_id(promise_expression);

        call_stack_elem_t stack_elem = STATE(fun_stack).back();
        info.in_call_id = get<0>(stack_elem);
        info.from_call_id = STATE(promise_origin)[info.prom_id];

        set_distances_and_lifestyle(info);

        info.prom_type = static_cast<sexp_type>(TYPEOF(PRCODE(promise_expression)));
        get_full_type(promise_expression, rho, info.full_type);
        info.return_type = static_cast<sexp_type>(TYPEOF(val));

        STATE(full_stack).pop_back();

        get_stack_parent(info, STATE(full_stack));
        info.in_prom_id = get_parent_promise();
        info.depth = get_no_of_ancestor_promises_on_stack();

        return info;
    }

    prom_info_t promise_lookup_get_info(const SEXP symbol, const SEXP rho, const SEXP val) {
        prom_info_t info;

        const char *name = get_name(symbol);
        if (name != NULL)
            info.name = name;

        SEXP promise_expression = get_promise(symbol, rho);
        info.prom_id = get_promise_id(promise_expression);

        call_stack_elem_t stack_elem = STATE(fun_stack).back();
        info.in_call_id = get<0>(stack_elem);
        info.from_call_id = STATE(promise_origin)[info.prom_id];

        set_distances_and_lifestyle(info);

        info.prom_type = static_cast<sexp_type>(TYPEOF(PRCODE(promise_expression)));
        info.full_type.push_back(sexp_type::OMEGA);
        info.return_type = static_cast<sexp_type>(TYPEOF(val));

        get_stack_parent(info, STATE(full_stack));
        info.in_prom_id = get_parent_promise();
        info.depth = get_no_of_ancestor_promises_on_stack();

        return info;
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

    DELEGATE(function_entry, closure_info_t)
    DELEGATE(function_exit, closure_info_t)
    DELEGATE(builtin_entry, builtin_info_t)
    DELEGATE(builtin_exit, builtin_info_t)
    DELEGATE(force_promise_entry, prom_info_t)
    DELEGATE(force_promise_exit, prom_info_t)
    DELEGATE(promise_created, prom_basic_info_t)
    DELEGATE(promise_lookup, prom_info_t)    
    DELEGATE(promise_lifecycle, prom_gc_info_t)
    DELEGATE(vector_alloc, type_gc_info_t)
    DELEGATE(gc_exit, gc_info_t)
    DELEGATE(gc_promise_unmarked)
    DELEGATE(init_recorder)
    DELEGATE(start_trace, metadata_t)
    DELEGATE(finish_trace, metadata_t)
    DELEGATE(unwind, unwind_info_t)

#undef DELEGATE
#undef DELEGATE1
#undef DELEGATE2
};

template<typename ...Rec>
class compose_impl;

template<typename ...Rec>
class compose : public compose_impl<Rec...> {
public:
    std::tuple<Rec...> rec;
};


template<typename ...Rec>
class compose_impl : public recorder_t<compose<Rec...>> {
protected:
    // Make sure no one can create an instance of compose_impl
    compose_impl() = default;
public:
#define COMPOSE2(func, info_struct) \
    void func(const info_struct & info) { \
        tuple_for_each(this->impl().rec, [&info](auto & r) { \
            r.func(info); \
        }); \
    }

#define COMPOSE1(func) \
    void func() { \
        tuple_for_each(this->impl().rec, [](auto & r) { \
            r.func(); \
        }); \
    }

#define COMPOSE(...) GET_MACRO(__VA_ARGS__, COMPOSE2, COMPOSE1)(__VA_ARGS__)

    COMPOSE(function_entry, closure_info_t)
    COMPOSE(function_exit, closure_info_t)
    COMPOSE(builtin_entry, builtin_info_t)
    COMPOSE(builtin_exit, builtin_info_t)
    COMPOSE(force_promise_entry, prom_info_t)
    COMPOSE(force_promise_exit, prom_info_t)
    COMPOSE(promise_created, prom_basic_info_t)
    COMPOSE(promise_lookup, prom_info_t)
    COMPOSE(promise_lifecycle, prom_gc_info_t)
    COMPOSE(vector_alloc, type_gc_info_t)
    COMPOSE(gc_exit, gc_info_t)
    COMPOSE(gc_promise_unmarked)
    COMPOSE(init_recorder)
    COMPOSE(start_trace, metadata_t)
    COMPOSE(finish_trace, metadata_t)
    COMPOSE(unwind, unwind_info_t)

#undef COMPOSE
#undef COMPOSE1
#undef COMPOSE2
#undef GET_MACRO
};

#endif //R_3_3_1_RECORDER_H
