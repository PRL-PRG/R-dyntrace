//
// Created by nohajc on 28.3.17.
//

#include "recorder.hpp"

void get_metadatum(metadata_t &metadata, string key) {
    char *value = getenv(key.c_str());
    if (value == NULL)
        metadata[key] = "";
    else
        metadata[key] = string(value);
}

void get_environment_metadata(metadata_t &metadata) {
    get_metadatum(metadata, "RDT_COMPILE_VIGNETTE");
    get_metadatum(metadata, "R_COMPILE_PKGS");
    get_metadatum(metadata, "R_DISABLE_BYTECODE");
    get_metadatum(metadata, "R_ENABLE_JIT");
    get_metadatum(metadata, "R_KEEP_PKG_SOURCE");
    get_metadatum(metadata, "R_ENABLE_JIT");
}

void get_current_time_metadata(metadata_t &metadata, string prefix) {
    chrono::time_point<chrono::system_clock> time_point =
        chrono::system_clock::now();
    time_t time = chrono::system_clock::to_time_t(time_point);
    string kludge = ctime(&time);
    metadata["RDT_TRACE_" + prefix + "_DATE"] =
        kludge.substr(0, kludge.length() - 1);
    metadata["RDT_TRACE_" + prefix + "_TIME"] =
        to_string(static_cast<long int>(time));
}

recursion_type is_recursive(fn_id_t function) {
    for (vector<call_stack_elem_t>::reverse_iterator i =
             tracer_state().fun_stack.rbegin();
         i != tracer_state().fun_stack.rend(); ++i) {
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

        if (cursor_type == function_type::BUILTIN ||
            cursor_type == function_type::CLOSURE) {
            return recursion_type::NOT_RECURSIVE;
        }

        // inside a different function, but one that doesn't matter, recursion
        // still
        // possible
    }
}

tuple<lifestyle_type, int, int>
judge_promise_lifestyle(call_id_t from_call_id) {
    int effective_distance = 0;
    int actual_distance = 0;
    for (vector<call_stack_elem_t>::reverse_iterator i =
             tracer_state().fun_stack.rbegin();
         i != tracer_state().fun_stack.rend(); ++i) {
        call_id_t cursor = get<0>(*i);
        function_type type = get<2>(*i);

        if (cursor == from_call_id)
            if (effective_distance == 0) {
                if (actual_distance == 0) {
                    return tuple<lifestyle_type, int, int>(
                        lifestyle_type::IMMEDIATE_LOCAL, effective_distance,
                        actual_distance);
                } else {
                    return tuple<lifestyle_type, int, int>(
                        lifestyle_type::LOCAL, effective_distance,
                        actual_distance);
                }
            } else {
                if (effective_distance == 1) {
                    return tuple<lifestyle_type, int, int>(
                        lifestyle_type::IMMEDIATE_BRANCH_LOCAL,
                        effective_distance, actual_distance);
                } else {
                    return tuple<lifestyle_type, int, int>(
                        lifestyle_type::BRANCH_LOCAL, effective_distance,
                        actual_distance);
                }
            }

        if (cursor == 0) {
            return tuple<lifestyle_type, int, int>(
                lifestyle_type::ESCAPED, -1,
                -1); // reached root, parent must be in a
                     // different branch--promise escaped
        }

        actual_distance++;
        if (type == function_type::BUILTIN || type == function_type::CLOSURE) {
            effective_distance++;
        }
    }
}

void set_distances_and_lifestyle(prom_info_t &info) {
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

void get_full_type_inner(SEXP sexp, SEXP rho, full_sexp_type &result,
                         set<SEXP> &visited) {
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
        //            bool try_to_attach_symbol_value = (rho != R_NilValue) ?
        //            isEnvironment(rho) : false;
        //            if (!try_to_attach_symbol_value) return;
        //
        //            SEXP uncompiled_sexp = BODY_EXPR(sexp);
        //            SEXP underlying_expression =
        //            findVar(PRCODE(uncompiled_sexp),
        //            rho);
        //
        //            if (underlying_expression == R_UnboundValue) return;
        //            if (underlying_expression == R_MissingArg) return;
        //
        //            PROTECT(underlying_expression);
        //            //get_full_type(underlying_expression, rho, result,
        //            visited);
        //           UNPROTECT(1);

        // Rprintf("hi from dbg\n`");
        return;
    }

    if (type == sexp_type::SYM) {
        bool try_to_attach_symbol_value =
            (rho != R_NilValue) ? isEnvironment(rho) : false;
        if (!try_to_attach_symbol_value)
            return;
        /* FIXME - findVar can eval an expression. This can fire another hook,
           leading to spurious data.
                   Reproduced below is a gdb backtrace obtained by running
           `dplyr`.
                   At #7, get_full_type_inner invokes `findVar` which leads to
           `eval` on #2.

           0x00000000004d4b3e in R_execClosure (call=0x94e1db0,
           newrho=0x94e1ec8,
           sysparent=0x1a05f80,
           rho=0x1a05f80, arglist=0x19d6b68, op=0x9534cb0) at eval.c:1612
           1612	    RDT_HOOK(probe_function_entry, call, op, newrho);
           (gdb) bt
           #0  0x00000000004d4b3e in R_execClosure (call=0x94e1db0,
           newrho=0x94e1ec8, sysparent=0x1a05f80,
           rho=0x1a05f80, arglist=0x19d6b68, op=0x9534cb0) at eval.c:1612
           #1  0x00000000004d49e9 in Rf_applyClosure (call=0x94e1db0,
           op=0x9534cb0,
           arglist=0x19d6b68,
           rho=0x1a05f80, suppliedvars=0x19d6b68) at eval.c:1583
           #2  0x00000000004d2db7 in Rf_eval (e=0x94e1db0, rho=0x1a05f80) at
           eval.c:776
           #3  0x00000000004bf6c9 in getActiveValue (fun=0x9534cb0) at
           envir.c:154
           #4  0x00000000004bfa53 in R_HashGet (hashcode=0, symbol=0x54de8f8,
           table=0x1e3a4328) at envir.c:281
           #5  0x00000000004c1781 in Rf_findVarInFrame3 (rho=0x953dd50,
           symbol=0x54de8f8, doGet=TRUE)
           at envir.c:1042
           #6  0x00000000004c1fb1 in Rf_findVar (symbol=0x54de8f8,
           rho=0x953dd50) at
           envir.c:1221
           #7  0x00007f3bcb98893a in
           recorder_t<psql_recorder_t>::get_full_type_inner (
           this=0x7f3bcbbde159 <trace_promises<psql_recorder_t>::rec_impl>,
           sexp=0x54de8f8, rho=0x9520500,
           result=std::vector of length 1, capacity 1 = {...}, visited=std::set
           with
           1 elements = {...})
           at
           /home/aviral/projects/aviral-r-dyntrace/rdt-plugins/promises/src/recorder.h:332
           #8  0x00007f3bcb98542f in recorder_t<psql_recorder_t>::get_full_type
           (
           this=0x7f3bcbbde159 <trace_promises<psql_recorder_t>::rec_impl>,
           promise=0x94e1d78, rho=0x9520500,
           result=std::vector of length 1, capacity 1 = {...})
           at
           /home/aviral/projects/aviral-r-dyntrace/rdt-plugins/promises/src/recorder.h:289
           #9  0x00007f3bcb980aeb in
           recorder_t<psql_recorder_t>::create_promise_get_info (
           this=0x7f3bcbbde159 <trace_promises<psql_recorder_t>::rec_impl>,
           promise=0x94e1d78, rho=0x9520500)
           at
           /home/aviral/projects/aviral-r-dyntrace/rdt-plugins/promises/src/recorder.h:352
         */
        SEXP symbol_points_to = R_UnboundValue; // findVar(sexp, rho);

        if (symbol_points_to == R_UnboundValue)
            return;
        if (symbol_points_to == R_MissingArg)
            return;
        // if (TYPEOF(symbol_points_to) == SYMSXP) return;

        get_full_type_inner(symbol_points_to, rho, result, visited);

        return;
    }
}

inline void get_full_type(SEXP promise, SEXP rho,
                          full_sexp_type &result) { // FIXME remove rho
    set<SEXP> visited;
    get_full_type_inner(PRCODE(promise), PRENV(promise), result, visited);
}

closure_info_t function_entry_get_info(const SEXP call, const SEXP op,
                                       const SEXP rho) {
    closure_info_t info;

    const char *name = get_name(call);
    const char *ns = get_ns_name(op);

    info.fn_compiled = is_byte_compiled(op);
    info.fn_type = function_type::CLOSURE;
    info.fn_id = get_function_id(op);
    info.fn_addr = get_function_addr(op);
    info.call_ptr = get_sexp_address(rho);
    info.call_id = make_funcall_id(op);
    // info.call_id = make_funcall_id(rho);

    call_stack_elem_t elem = tracer_state().fun_stack.back();
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

    get_stack_parent(info, tracer_state().full_stack);
    info.in_prom_id = get_parent_promise();

    stack_event_t stack_elem;
    stack_elem.type = stack_type::CALL;
    stack_elem.call_id = info.call_id;
    tracer_state().full_stack.push_back(stack_elem);

    return info;
}

closure_info_t function_exit_get_info(const SEXP call, const SEXP op,
                                      const SEXP rho) {
    closure_info_t info;

    const char *name = get_name(call);
    const char *ns = get_ns_name(op);

    info.fn_compiled = is_byte_compiled(op);
    info.fn_id = get_function_id(op);
    info.fn_addr = get_function_addr(op);
    call_stack_elem_t elem = tracer_state().fun_stack.back();
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

    tracer_state().fun_stack.pop_back();
    call_stack_elem_t elem_parent = tracer_state().fun_stack.back();
    info.parent_call_id = get<0>(elem_parent);

    info.recursion = is_recursive(info.fn_id);

    tracer_state().full_stack.pop_back();
    get_stack_parent(info, tracer_state().full_stack);
    info.in_prom_id = get_parent_promise();

    return info;
}

builtin_info_t builtin_entry_get_info(const SEXP call, const SEXP op,
                                      const SEXP rho, function_type fn_type) {
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

    // R_FunTab[PRIMOFFSET(op)].eval % 100 )/10 ==

    call_stack_elem_t elem = tracer_state().fun_stack.back();
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

    get_stack_parent(info, tracer_state().full_stack);
    info.in_prom_id = get_parent_promise();

    stack_event_t stack_elem;
    stack_elem.type = stack_type::CALL;
    stack_elem.call_id = info.call_id;
    tracer_state().full_stack.push_back(stack_elem);

    return info;
}

builtin_info_t builtin_exit_get_info(const SEXP call, const SEXP op,
                                     const SEXP rho, function_type fn_type) {
    builtin_info_t info;

    const char *name = get_name(call);
    if (name != NULL)
        info.name = name;
    info.fn_id = get_function_id(op);
    info.fn_addr = get_function_addr(op);
    call_stack_elem_t elem = tracer_state().fun_stack.back();
    info.call_id = get<0>(elem);
    if (name != NULL)
        info.name = name;
    info.fn_type = fn_type;
    info.fn_compiled = is_byte_compiled(op);
    info.fn_definition = get_expression(op);

    call_stack_elem_t parent_elem = tracer_state().fun_stack.back();
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

    tracer_state().full_stack.pop_back();
    get_stack_parent(info, tracer_state().full_stack);
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

prom_basic_info_t create_promise_get_info(const SEXP promise, const SEXP rho) {
    prom_basic_info_t info;

    info.prom_id = make_promise_id(promise);
    tracer_state().fresh_promises.insert(info.prom_id);

    info.prom_type = static_cast<sexp_type>(TYPEOF(PRCODE(promise)));
    get_full_type(promise, rho, info.full_type);

    get_stack_parent(info, tracer_state().full_stack);
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

    call_stack_elem_t call_stack_elem = tracer_state().fun_stack.back();
    info.in_call_id = get<0>(call_stack_elem);
    info.from_call_id = tracer_state().promise_origin[info.prom_id];

    set_distances_and_lifestyle(info);

    info.prom_type = static_cast<sexp_type>(TYPEOF(PRCODE(promise_expression)));
    get_full_type(promise_expression, rho, info.full_type);
    info.return_type = sexp_type::OMEGA;

    get_stack_parent(info, tracer_state().full_stack);
    info.in_prom_id = get_parent_promise();
    info.depth = get_no_of_ancestor_promises_on_stack();

    stack_event_t stack_elem;
    stack_elem.type = stack_type::PROMISE;
    stack_elem.promise_id = info.prom_id;
    tracer_state().full_stack.push_back(stack_elem);

    return info;
}

prom_info_t force_promise_exit_get_info(const SEXP symbol, const SEXP rho,
                                        const SEXP val) {
    prom_info_t info;

    const char *name = get_name(symbol);
    if (name != NULL)
        info.name = name;

    SEXP promise_expression = get_promise(symbol, rho);
    info.prom_id = get_promise_id(promise_expression);

    call_stack_elem_t stack_elem = tracer_state().fun_stack.back();
    info.in_call_id = get<0>(stack_elem);
    info.from_call_id = tracer_state().promise_origin[info.prom_id];

    set_distances_and_lifestyle(info);

    info.prom_type = static_cast<sexp_type>(TYPEOF(PRCODE(promise_expression)));
    get_full_type(promise_expression, rho, info.full_type);
    info.return_type = static_cast<sexp_type>(TYPEOF(val));

    tracer_state().full_stack.pop_back();

    get_stack_parent(info, tracer_state().full_stack);
    info.in_prom_id = get_parent_promise();
    info.depth = get_no_of_ancestor_promises_on_stack();

    return info;
}

prom_info_t promise_lookup_get_info(const SEXP symbol, const SEXP rho,
                                    const SEXP val) {
    prom_info_t info;

    const char *name = get_name(symbol);
    if (name != NULL)
        info.name = name;

    SEXP promise_expression = get_promise(symbol, rho);
    info.prom_id = get_promise_id(promise_expression);

    call_stack_elem_t stack_elem = tracer_state().fun_stack.back();
    info.in_call_id = get<0>(stack_elem);
    info.from_call_id = tracer_state().promise_origin[info.prom_id];

    set_distances_and_lifestyle(info);

    info.prom_type = static_cast<sexp_type>(TYPEOF(PRCODE(promise_expression)));
    info.full_type.push_back(sexp_type::OMEGA);
    info.return_type = static_cast<sexp_type>(TYPEOF(val));

    get_stack_parent(info, tracer_state().full_stack);
    info.in_prom_id = get_parent_promise();
    info.depth = get_no_of_ancestor_promises_on_stack();

    return info;
}

prom_info_t promise_expression_lookup_get_info(const SEXP prom,
                                               const SEXP rho) {
    prom_info_t info;

    info.prom_id = get_promise_id(prom);

    call_stack_elem_t stack_elem = tracer_state().fun_stack.back();
    info.in_call_id = get<0>(stack_elem);
    info.from_call_id = tracer_state().promise_origin[info.prom_id];

    set_distances_and_lifestyle(info);

    info.prom_type = static_cast<sexp_type>(TYPEOF(PRCODE(prom)));
    info.full_type.push_back(sexp_type::OMEGA);
    info.return_type = static_cast<sexp_type>(TYPEOF(PRCODE(prom)));

    return info;
}
