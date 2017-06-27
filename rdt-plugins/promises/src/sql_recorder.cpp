//
// Created by nohajc on 29.3.17.
//

#include <cstring>
#include <cstdio>

#include "sql_recorder.h"
#include "tracer_conf.h"

#include "sql_generator.h"
#include "multiplexer.h"
#include "tools.h"

#include <string>

using namespace std;
using namespace sql_generator;

typedef int prom_eval_t;

/* Functions for generating SQL strings. */

sql_stmt_t insert_function_statement(const call_info_t & info) {
    sql_val_t id = from_int(info.fn_id);
    sql_val_t location = wrap_nullable_string(info.loc);
    sql_val_t definition = wrap_and_escape_nullable_string(info.fn_definition);
    sql_val_t type = from_int(tools::enum_cast(info.fn_type));
    sql_val_t compiled = from_int(info.fn_compiled ? 1 : 0);

    return make_insert_function_statement(id, location, definition, type, compiled);
}

sql_stmt_t insert_arguments_statement(const closure_info_t & info, bool align) {
    assert(info.arguments.size() > 0);

    vector<sql_val_cell_t> value_cells;
    sql_val_t call_id = from_int(info.call_id);

    int i = 0;
    for (auto argument_ref : info.arguments.all()) {
        const arg_t &argument = argument_ref.get();
        sql_val_t argument_name = wrap_nullable_string(get<0>(argument));
        sql_val_t argument_id = from_int(get<1>(argument));
        sql_val_t index = from_int(i++);

        sql_val_cell_t cell = join({argument_id, argument_name, index, call_id});
        value_cells.push_back(cell);
    }

    return make_insert_arguments_statement(value_cells, align);
}

sql_stmt_t insert_call_statement(const call_info_t & info) {
    sql_val_t id = from_int(info.call_id);
    sql_val_t pointer = from_hex(info.call_ptr); // FIXME do we really need this?
    sql_val_t name = wrap_nullable_string(info.name);
    sql_val_t function_id = from_int(info.fn_id);
    sql_val_t parent_call_id = from_int(info.parent_call_id);
    sql_val_t callsite = wrap_nullable_string(info.callsite);

    return make_insert_function_call_statement(id, name, callsite, function_id, parent_call_id);
}

sql_stmt_t insert_promise_statement(const prom_basic_info_t & info) {
    return make_insert_promise_statement(
            from_int(info.prom_id),
            from_int(tools::enum_cast(info.prom_type)),
            (info.prom_type == sexp_type::BCODE)
                ? from_int(tools::enum_cast(info.prom_original_type))
                : "null",
            (info.prom_type == sexp_type::SYM || info.prom_type == sexp_type::BCODE && info.prom_type == sexp_type::SYM)
                ? from_int(tools::enum_cast(info.symbol_underlying_type))
                : "null"
    );
}

sql_stmt_t insert_promise_association_statement(const closure_info_t & info, bool align) {
    assert(info.arguments.size() > 0);

    vector<sql_val_cell_t> value_cells;
    sql_val_t call_id = from_int(info.call_id);

    int i = 0;
    for (auto argument_ref : info.arguments.all()) {
        const arg_t &argument = argument_ref.get();
        //sql_val_t argument_name = get<0>(argument);
        sql_val_t argument_id = from_int(get<1>(argument));
        sql_val_t promise_id = from_int(get<2>(argument));
        sql_val_t index = from_int(i++);

        sql_val_cell_t cell = join({promise_id, call_id, argument_id});
        value_cells.push_back(cell);
    }

    return make_insert_promise_associations_statement(value_cells, align);
}

sql_stmt_t insert_promise_evaluation_statement(prom_eval_t type, const prom_info_t & info) {
    //sql_val_t clock = from_int(STATE(clock_id)++);
    sql_val_t clock = next_from_sequence();
    sql_val_t event_type = from_int(type);
    sql_val_t promise_id = from_int(info.prom_id);
    sql_val_t from_call_id = from_int(info.from_call_id);
    sql_val_t in_call_id = from_int(info.in_call_id);
    sql_val_t lifestyle = from_int(tools::enum_cast(info.lifestyle));
    sql_val_t effective_distance_from_origin = from_int(info.effective_distance_from_origin);
    sql_val_t actual_distance_from_origin = from_int(info.actual_distance_from_origin);

    // in_call_id = current call
    // from_call_id = TODO what is it

    return make_insert_promise_evaluation_statement(clock, event_type, promise_id, from_call_id, in_call_id, lifestyle, effective_distance_from_origin, actual_distance_from_origin);
}

/* Functions connecting to the outside world, create SQL and multiplex output. */

void sql_recorder_t::function_entry(const closure_info_t & info) {
    bool align_statements = tracer_conf.pretty_print;
    bool need_to_insert = register_inserted_function(info.fn_id);

    if (need_to_insert) {
        sql_stmt_t statement = insert_function_statement(info);
        multiplexer::output(
                multiplexer::payload_t(statement),
                tracer_conf.outputs);
    }

    if (info.arguments.size() > 0) {
        sql_stmt_t statement = insert_arguments_statement(info, align_statements);
        multiplexer::output(
                multiplexer::payload_t(statement),
                tracer_conf.outputs);
    }

    /* always */ {
        sql_stmt_t statement = insert_call_statement(info);
        multiplexer::output(
                multiplexer::payload_t(statement),
                tracer_conf.outputs);
    }

    if (info.arguments.size() > 0) {
        sql_stmt_t statement = insert_promise_association_statement(info, align_statements);
        multiplexer::output(
                multiplexer::payload_t(statement),
                tracer_conf.outputs);
    }
}

void sql_recorder_t::builtin_entry(const builtin_info_t & info) {
    bool need_to_insert = register_inserted_function(info.fn_id);

    if (need_to_insert) {
        sql_stmt_t statement = insert_function_statement(info);
                multiplexer::output(
                multiplexer::payload_t(statement),
                tracer_conf.outputs);
    }

    /* always */ {
        sql_stmt_t statement = insert_call_statement(info);
                multiplexer::output(
                multiplexer::payload_t(statement),
                tracer_conf.outputs);
    }

    // We do not handle arguments for built-ins.
}

void sql_recorder_t::force_promise_entry(const prom_info_t & info) {
    if (info.prom_id < 0) // if this is a promise from the outside
        if (!negative_promise_already_inserted(info.prom_id)) {
            sql_stmt_t statement = insert_promise_statement(info);
            multiplexer::output(
                    multiplexer::payload_t(statement),
                    tracer_conf.outputs);
        }

    /* always */ {
        sql_stmt_t statement = insert_promise_evaluation_statement(RDT_SQL_FORCE_PROMISE, info);
        multiplexer::output(
                multiplexer::payload_t(statement),
                tracer_conf.outputs);
    }
}

void sql_recorder_t::promise_created(const prom_basic_info_t & info) {
    sql_stmt_t statement = insert_promise_statement(info);
    multiplexer::output(
            multiplexer::payload_t(statement),
            tracer_conf.outputs);
}

void sql_recorder_t::promise_lookup(const prom_info_t & info) {
    sql_stmt_t statement = insert_promise_evaluation_statement(RDT_SQL_LOOKUP_PROMISE, info);
    multiplexer::output(
            multiplexer::payload_t(statement),
            tracer_conf.outputs);
}

void sql_recorder_t::start_trace() { // bool output_configuration
    multiplexer::init(tracer_conf.outputs, tracer_conf.filename, tracer_conf.overwrite);

    if (tracer_conf.include_configuration)
        if (tracer_conf.overwrite) {
            sql_stmt_t pragma_statement = make_pragma_statement("synchronous", "off");

            sql_stmt_t create_functions_statement = make_create_functions_statement();
            sql_stmt_t create_calls_statement = make_create_calls_statement();
            sql_stmt_t create_arguments_statement = make_create_arguments_statement();
            sql_stmt_t create_promises_statement = make_create_promises_statement();
            sql_stmt_t create_associations_statement = make_create_promise_associations_statement();
            sql_stmt_t create_evaluations_statement = make_create_promise_evaluations_statement();

            multiplexer::output(
                    multiplexer::payload_t(pragma_statement),
                    tracer_conf.outputs);

            multiplexer::output(
                    multiplexer::payload_t(create_calls_statement),
                    tracer_conf.outputs);

            multiplexer::output(
                    multiplexer::payload_t(create_arguments_statement),
                    tracer_conf.outputs);

            multiplexer::output(
                    multiplexer::payload_t(create_promises_statement),
                    tracer_conf.outputs);

            multiplexer::output(
                    multiplexer::payload_t(create_associations_statement),
                    tracer_conf.outputs);

            multiplexer::output(
                    multiplexer::payload_t(create_evaluations_statement),
                    tracer_conf.outputs);
        }

    if (!tracer_conf.overwrite && tracer_conf.reload_state) {
        sql_stmt_t max_function_id_query = make_select_max_function_id_statement();
        sql_stmt_t max_call_id_query = make_select_max_call_id_statement();
        sql_stmt_t max_promise_id_query = make_select_max_promise_id_statement();
        sql_stmt_t min_promise_id_query = make_select_min_promise_id_statement();
        sql_stmt_t max_argument_id_query = make_select_max_argument_id_statement();
        sql_stmt_t max_clock_query = make_select_max_promise_evaluation_clock_statement();
        sql_stmt_t functions_query = make_select_all_function_ids_and_definitions_statement();
        sql_stmt_t arguments_query = make_select_all_argument_ids_names_and_function_allegiances_statement();
        sql_stmt_t already_inserted_functions_query = make_select_all_function_ids_statement();
        sql_stmt_t already_inserted_negative_promises_query = make_select_all_negative_promise_ids_statement();

        multiplexer::int_result max_function_id;
        multiplexer::int_result max_call_id;
        multiplexer::int_result max_clock;
        multiplexer::int_result max_promise_id;
        multiplexer::int_result min_promise_id;
        multiplexer::int_result max_argument_id;
        multiplexer::string_to_int_map_result functions;
        multiplexer::ulong_string_to_ulong_map_result arguments;

        multiplexer::int_set_result already_inserted_functions;
        multiplexer::int_set_result already_inserted_negative_promises;

        multiplexer::input(multiplexer::payload_t(max_function_id_query), tracer_conf.outputs, max_function_id);
        multiplexer::input(multiplexer::payload_t(max_call_id_query), tracer_conf.outputs, max_call_id);
        multiplexer::input(multiplexer::payload_t(max_clock_query), tracer_conf.outputs, max_clock);
        multiplexer::input(multiplexer::payload_t(max_promise_id_query), tracer_conf.outputs, max_promise_id);
        multiplexer::input(multiplexer::payload_t(min_promise_id_query), tracer_conf.outputs, min_promise_id);
        multiplexer::input(multiplexer::payload_t(max_argument_id_query), tracer_conf.outputs, max_argument_id);
        multiplexer::input(multiplexer::payload_t(functions_query), tracer_conf.outputs, functions);
        multiplexer::input(multiplexer::payload_t(arguments_query), tracer_conf.outputs, arguments);
        multiplexer::input(multiplexer::payload_t(already_inserted_functions_query), tracer_conf.outputs, already_inserted_functions);
        multiplexer::input(multiplexer::payload_t(already_inserted_negative_promises_query), tracer_conf.outputs, already_inserted_negative_promises);

        STATE(clock_id) = max_clock.result + 1;
        STATE(call_id_counter) = max_call_id.result + 1;
        STATE(fn_id_counter) = max_function_id.result + 1;
        STATE(prom_id_counter) = max_promise_id.result + 1;
        STATE(prom_neg_id_counter) = min_promise_id.result;
        STATE(argument_id_sequence) = max_argument_id.result + 1;
        STATE(function_ids) = functions.result;
        STATE(argument_ids) = arguments.result;
        STATE(already_inserted_functions) = already_inserted_functions.result;
        STATE(already_inserted_negative_promises) = already_inserted_negative_promises.result;
    }

    /* always */ {
        sql_stmt_t statement = make_begin_transaction_statement();
        multiplexer::output(
                multiplexer::payload_t(statement),
                tracer_conf.outputs);
    }
}

void sql_recorder_t::finish_trace() {
    sql_stmt_t statement = make_commit_transaction_statement();
    multiplexer::output(
            multiplexer::payload_t(statement),
            tracer_conf.outputs);

    multiplexer::close(tracer_conf.outputs);
}
