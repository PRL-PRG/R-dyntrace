//
// Created by nohajc on 29.3.17.
//

#include <cstring>

#include "trace_recorder.h"
#include "tracer_output.h"
#include "tracer_conf.h"
#include "tracer_state.h"

void trace_recorder_t::function_entry(const call_info_t & info) {
    // TODO: this can be simplified - we don't need to check output format inside rdt_print
    rdt_print(OutputFormat::RDT_OUTPUT_TRACE, {print_function(info.type.c_str(), info.loc.c_str(), info.fqfn.c_str(), info.fn_id, info.call_id, info.arguments)});

    if (tracer_conf.pretty_print)
        STATE(indent) += tracer_conf.indent_width;
}

void trace_recorder_t::function_exit(const call_info_t & info) {
    if (tracer_conf.pretty_print)
        STATE(indent) -= tracer_conf.indent_width;

    rdt_print(OutputFormat::RDT_OUTPUT_TRACE, {print_function(info.type.c_str(), info.loc.c_str(), info.fqfn.c_str(), info.fn_id, info.call_id, info.arguments)});
}

void trace_recorder_t::builtin_entry(const call_info_t & info) {
    // TODO merge rdt_print_calls
    rdt_print(OutputFormat::RDT_OUTPUT_TRACE, {print_builtin("=> b-in", NULL, info.name.c_str(), info.fn_id, info.call_id)});

    if (tracer_conf.pretty_print)
        STATE(indent) += tracer_conf.indent_width;
}

void trace_recorder_t::builtin_exit(const call_info_t & info) {
    if (tracer_conf.pretty_print)
        STATE(indent) -= tracer_conf.indent_width;

    rdt_print(OutputFormat::RDT_OUTPUT_TRACE, {print_builtin("<= b-in", NULL, info.name.c_str(), info.fn_id, info.call_id)});
}

void trace_recorder_t::force_promise_entry(const prom_info_t & info) {
    rdt_print(OutputFormat::RDT_OUTPUT_TRACE, {print_promise("=> prom", NULL, info.name.c_str(), info.prom_id, info.in_call_id, info.from_call_id)});
}

void trace_recorder_t::force_promise_exit(const prom_info_t & info) {
    rdt_print(OutputFormat::RDT_OUTPUT_TRACE, {print_promise("<= prom", NULL, info.name.c_str(), info.prom_id, info.in_call_id, info.from_call_id)});
}

void trace_recorder_t::promise_created(const prom_id_t & prom_id) {
    //Rprintf("PROMISE CREATED at %p\n", get_sexp_address(prom));
    //TODO implement promise allocation pretty print
    //rdt_print(RDT_OUTPUT_TRACE, {print_promise_alloc(prom_id)});
}

void trace_recorder_t::promise_lookup(const prom_info_t & info) {
    rdt_print(OutputFormat::RDT_OUTPUT_TRACE, {print_promise("<> lkup", NULL, info.name.c_str(), info.prom_id, info.in_call_id, info.from_call_id)});
}
