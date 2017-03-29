//
// Created by nohajc on 28.3.17.
//

#ifndef R_3_3_1_RECORDER_H
#define R_3_3_1_RECORDER_H

#include <tuple>
#include "tuple_for_each.h"

#include "tracer_sexpinfo.h"

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
        info.loc = CHKSTR(location);
        free(location);

        if (ns) {
            info.fqfn = string(ns) + "::" + CHKSTR(name);
        } else {
            info.fqfn = CHKSTR(name);
        }

        info.arguments = get_arguments(op, rho);
        info.fn_definition = get_expression(op);

        return info;
    }

    void function_entry_process(const call_info_t & info) {
        impl().function_entry(info);
    }

    // TODO: merge duplicate code from function_entry/exit
    call_info_t function_exit_get_info(const SEXP call, const SEXP op, const SEXP rho) {
        call_info_t info;

        const char *name = get_name(call);
        const char *ns = get_ns_name(op);

        info.type = is_byte_compiled(call) ? "<= bcod" : "<= func";
        info.fn_id = get_function_id(op);
        info.call_id = STATE(fun_stack).top();

        char *location = get_location(op);
        info.loc = CHKSTR(location);
        free(location);

        if (ns) {
            info.fqfn = string(ns) + "::" + CHKSTR(name);
        } else {
            info.fqfn = CHKSTR(name);
        }

        info.arguments = get_arguments(op, rho);

        return info;
    }

    void function_exit_process(const call_info_t & info) {
        impl().function_exit(info);
    }
};




// TODO: Move individual recorder classes to separate headers
class trace_recorder_t : public recorder_t<trace_recorder_t> {
public:
    // TODO: Move these function definitions to .cpp files
    void function_entry(const call_info_t & info) {
        // TODO: this can be simplified - we don't need to check output format inside rdt_print
        rdt_print(RDT_OUTPUT_TRACE, {print_function(info.type.c_str(), info.loc.c_str(), info.fqfn.c_str(), info.fn_id, info.call_id, info.arguments)});

        if (tracer_conf.pretty_print)
            STATE(indent) += tracer_conf.indent_width;

    }

    void function_exit(const call_info_t & info) {
        if (tracer_conf.pretty_print)
            STATE(indent) -= tracer_conf.indent_width;

        rdt_print(RDT_OUTPUT_TRACE, {print_function(info.type.c_str(), info.loc.c_str(), info.fqfn.c_str(), info.fn_id, info.call_id, info.arguments)});
    }
};

class sql_recorder_t : public recorder_t<sql_recorder_t> {
public:
    void function_entry(const call_info_t & info) {
        // TODO link with mk_sql
        // TODO rename to reflect non-printing nature
        // TODO meh, ugly
        if (tracer_conf.output_format == RDT_OUTPUT_COMPILED_SQLITE && tracer_conf.output_type == RDT_SQLITE) {
            // TODO: Change function signatures to accept std::string instead of const char*
            run_prep_sql_function(info.fn_id, info.arguments, info.loc.c_str(), info.fn_definition.c_str());
            run_prep_sql_function_call(info.call_id, info.call_ptr, info.fqfn.c_str(), info.loc.c_str(), info.call_type, info.fn_id);
            run_prep_sql_promise_assoc(info.arguments, info.call_id);
        } else {
            rdt_print(RDT_OUTPUT_SQL, {mk_sql_function(info.fn_id, info.arguments, info.loc.c_str(), info.fn_definition.c_str()),
                                       mk_sql_function_call(info.call_id, info.call_ptr, info.fqfn.c_str(), info.loc.c_str(), info.call_type, info.fn_id),
                                       mk_sql_promise_assoc(info.arguments, info.call_id)});
        }
    }

    void function_exit(const call_info_t & info) {}
};

template<typename ...Rec>
class compose : public recorder_t<compose<Rec...>> {
    std::tuple<Rec...> rec;

public:
#define COMPOSE(func) \
    void func(const call_info_t & info) { \
        tuple_for_each(rec, [&info](auto & r) { \
            r.func(info); \
        }); \
    }

    COMPOSE(function_entry)
    COMPOSE(function_exit)

#undef COMPOSE
};

#endif //R_3_3_1_RECORDER_H
