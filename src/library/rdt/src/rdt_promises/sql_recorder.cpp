//
// Created by nohajc on 29.3.17.
//

#include <stdio.h>

#include "sql_recorder.h"
#include "tracer_conf.h"
#include "tracer_output.h"

#include "sql_generator.h"

#include <string>

using namespace std;

inline string populate(sql_generator::sql_stmt_t statement, ...) {
    char *stupid_c_string;

    va_list args;
    va_start(args, fmt);
    vasprintf(&stupid_c_string, statement.c_str(), args);
    va_end(args);

    return stupid_c_string;
}

sql_generator::sql_stmt_t insert_function_statement(const call_info_t & info) {
    string id = to_string(info.fn_id);
    string location = info.loc == nullptr ? "null" : info.loc;
    string definition = info.fn_definition == nullptr ? "null" : info.fn_definition;
    return populate(sql_generator::insert_function_template(), id, location, definition);
}

sql_generator::sql_stmt_t insert_arguments_statement(const call_info_t & info) {
    int num_of_arguments = info.arguments.size();

    for (auto argument_ref : info.arguments.all()) {
        const arg_t & argument = argument_ref.get();
    }

    return populate(sql_generator::insert_arguments_template(num_of_arguments));
}

sql_generator::sql_stmt_t insert_call_statement(const call_info_t & info) {
    string id = to_string(info.fn_id);
    string location = info.loc == nullptr ? "null" : info.loc;
    string definition = info.fn_definition == nullptr ? "null" : info.fn_definition;
    return populate(sql_generator::insert_function_template(), id, location, definition);
}

void sql_recorder_t::function_entry(const call_info_t & info) {

    if (STATE(already_inserted_functions).count(info.fn_id) == 0)
        Rprintf(insert_function_statement(info).c_str()); // TODO

    if (info.arguments.size() > 0)
        Rprintf(insert_arguments_statement(info).c_str());


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
