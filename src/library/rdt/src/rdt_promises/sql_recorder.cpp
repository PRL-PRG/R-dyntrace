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
using namespace sql_generator;

inline string populate(sql_generator::sql_stmt_t statement, ...) {
    char *stupid_c_string;

    va_list args;
    va_start(args, fmt);
    vasprintf(&stupid_c_string, statement.c_str(), args);
    va_end(args);

    return stupid_c_string;
}

sql_stmt_t insert_function_statement(const call_info_t & info) {
    sql_val_t id = from_int(info.fn_id);
    sql_val_t location = from_nullable_string(info.loc);
    sql_val_t definition = from_nullable_string(info.fn_definition);

    return make_insert_function_statement(id, location, definition);
}

sql_stmt_t insert_arguments_statement(const call_info_t & info) {
    assert(info.arguments.size() > 0);

    vector<sql_val_cell_t> value_cells;
    sql_val_t function_id = from_int(info.fn_id);

    int i = 0;
    for (auto argument_ref : info.arguments.all()) {
        const arg_t &argument = argument_ref.get();
        sql_val_t argument_name = get<0>(argument);
        sql_val_t argument_id = from_int(get<1>(argument));
        sql_val_t index = from_int(i++);

        sql_val_cell_t cell = join({argument_id, argument_name, index, function_id});
        value_cells.push_back(cell);
    }

    return make_insert_arguments_statement(value_cells);
}

sql_stmt_t insert_call_statement(const call_info_t & info) {
    sql_val_t id = from_int(info.call_id);
    sql_val_t pointer = from_hex(info.call_ptr); // FIXME do we really need this?
    sql_val_t name = from_nullable_string(info.fqfn);
    sql_val_t type = info.type;
    sql_val_t location = from_nullable_string(info.loc);
    sql_val_t function_id = from_int(info.fn_id);

    return make_insert_function_call_statement(id, pointer, name, type, location, function_id);
}

void sql_recorder_t::function_entry(const call_info_t & info) {

    if (STATE(already_inserted_functions).count(info.fn_id) == 0)
        Rprintf(insert_function_statement(info).c_str()); // TODO

    if (info.arguments.size() > 0)
        Rprintf(insert_arguments_statement(info).c_str());

    Rprintf(insert_call_statement(info).c_str());

    // TODO remove
//    1
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
