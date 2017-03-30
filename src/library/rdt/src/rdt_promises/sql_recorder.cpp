//
// Created by nohajc on 29.3.17.
//

#include <cstring>

#include "sql_recorder.h"
#include "tracer_conf.h"
#include "tracer_output.h"

// TODO: remove duplicities in code

void sql_recorder_t::function_entry(const call_info_t & info) {
    // TODO link with mk_sql
    // TODO rename to reflect non-printing nature
    // TODO meh, ugly
    if (tracer_conf.output_format == OutputFormat::RDT_OUTPUT_COMPILED_SQLITE && tracer_conf.output_type == OutputType::RDT_SQLITE) {
        // TODO: Change function signatures to accept std::string instead of const char*
        run_prep_sql_function(info.fn_id, info.arguments, info.loc.c_str(), info.fn_definition.c_str());
        run_prep_sql_function_call(info.call_id, info.call_ptr, info.fqfn.c_str(), info.loc.c_str(), info.call_type, info.fn_id);
        run_prep_sql_promise_assoc(info.arguments, info.call_id);
    } else {
        rdt_print(OutputFormat::RDT_OUTPUT_SQL, {mk_sql_function(info.fn_id, info.arguments, info.loc.c_str(), info.fn_definition.c_str()),
                                   mk_sql_function_call(info.call_id, info.call_ptr, info.fqfn.c_str(), info.loc.c_str(), info.call_type, info.fn_id),
                                   mk_sql_promise_assoc(info.arguments, info.call_id)});
    }
}

void sql_recorder_t::builtin_entry(const call_info_t & info) {
    if (tracer_conf.output_format == OutputFormat::RDT_OUTPUT_COMPILED_SQLITE && tracer_conf.output_type == OutputType::RDT_SQLITE) {
        run_prep_sql_function(info.fn_id, info.arguments, NULL, NULL);
        run_prep_sql_function_call(info.call_id, info.call_ptr, info.name.c_str(), NULL, 1, info.fn_id);
        //run_prep_sql_promise_assoc(arguments, call_id);
    } else {
        rdt_print(OutputFormat::RDT_OUTPUT_SQL, {mk_sql_function(info.fn_id, info.arguments, NULL, NULL),
                                   mk_sql_function_call(info.call_id, info.call_ptr, info.name.c_str(), NULL, 1, info.fn_id)});
    }
}

void sql_recorder_t::force_promise_entry(const prom_info_t & info) {
    // in_call_id = current call
    if (tracer_conf.output_format == OutputFormat::RDT_OUTPUT_COMPILED_SQLITE && tracer_conf.output_type == OutputType::RDT_SQLITE) {
        run_prep_sql_promise_evaluation(RDT_FORCE_PROMISE, info.prom_id, info.from_call_id);
    } else {
        rdt_print(OutputFormat::RDT_OUTPUT_SQL, {mk_sql_promise_evaluation(RDT_FORCE_PROMISE, info.prom_id, info.from_call_id)});
    }
}

void sql_recorder_t::promise_created(const prom_id_t & prom_id) {
    if (tracer_conf.output_format == OutputFormat::RDT_OUTPUT_COMPILED_SQLITE && tracer_conf.output_type == OutputType::RDT_SQLITE) {
        run_prep_sql_promise(prom_id);
    } else {
        rdt_print(OutputFormat::RDT_OUTPUT_SQL, {mk_sql_promise(prom_id)});
    }
}

void sql_recorder_t::promise_lookup(const prom_info_t & info) {
    // TODO
    if (tracer_conf.output_format == OutputFormat::RDT_OUTPUT_COMPILED_SQLITE && tracer_conf.output_type == OutputType::RDT_SQLITE) {
        run_prep_sql_promise_evaluation(RDT_LOOKUP_PROMISE, info.prom_id, info.from_call_id);
    } else {
        rdt_print(OutputFormat::RDT_OUTPUT_SQL, {mk_sql_promise_evaluation(RDT_LOOKUP_PROMISE, info.prom_id, info.from_call_id)});
    }
}
