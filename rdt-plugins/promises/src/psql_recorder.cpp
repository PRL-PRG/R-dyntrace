//
// Created by nohajc on 29.3.17.
//

#include <stdio.h>

#include "psql_recorder.h"
#include "tracer_conf.h"

#include "multiplexer.h"
#include "sql_generator.h"
#include "tools.h"

#ifdef RDT_SQLITE_SUPPORT
#endif

#include <string>

using namespace std;
using namespace sql_generator;

typedef int prom_eval_t;

#ifdef RDT_SQLITE_SUPPORT
// Helper functions.
sqlite3_stmt* compile_sql_statement(sql_stmt_t statement);

// Prepared statement objects for schema;
static vector<sqlite3_stmt *> prepared_sql_create_tables_and_views;

// Prepared statement objects for queries.
static sqlite3_stmt * max_function_id_query = nullptr;
static sqlite3_stmt * max_call_id_query = nullptr;
static sqlite3_stmt * max_promise_id_query = nullptr;
static sqlite3_stmt * min_promise_id_query = nullptr;
static sqlite3_stmt * max_argument_id_query = nullptr;
static sqlite3_stmt * max_clock_query = nullptr;
static sqlite3_stmt * functions_query = nullptr;
static sqlite3_stmt * arguments_query = nullptr;
static sqlite3_stmt * already_inserted_functions_query = nullptr;
static sqlite3_stmt * already_inserted_negative_promises_query = nullptr;

// Prepared statement objects for updates.
static sqlite3_stmt * prepared_sql_insert_metadata = nullptr;
static sqlite3_stmt * prepared_sql_insert_function = nullptr;
static sqlite3_stmt * prepared_sql_insert_call = nullptr;
static sqlite3_stmt * prepared_sql_insert_promise = nullptr;
static sqlite3_stmt * prepared_sql_insert_promise_eval = nullptr;
static sqlite3_stmt * prepared_sql_insert_promise_return = nullptr;
static sqlite3_stmt * prepared_sql_insert_promise_lifecycle = nullptr;
static sqlite3_stmt * prepared_sql_insert_gc_trigger = nullptr;
static sqlite3_stmt * prepared_sql_insert_type_distribution = nullptr;
static sqlite3_stmt * prepared_sql_transaction_begin = nullptr;
static sqlite3_stmt * prepared_sql_transaction_commit = nullptr;
static sqlite3_stmt * prepared_sql_transaction_abort = nullptr;
static sqlite3_stmt * prepared_sql_pragma_asynchronous = nullptr;

// Prepared statement object getters for objects with variable arrity.
sqlite3_stmt *get_prepared_sql_insert_argument(int);
sqlite3_stmt *get_prepared_sql_insert_promise_assoc(int);

// Memoization for above.
typedef map<int, sqlite3_stmt *> pstmt_cache;
pstmt_cache prepared_sql_insert_promise_assocs;
pstmt_cache prepared_sql_insert_arguments;

// More helper functions.
void free_prepared_sql_statements();
void free_prepared_sql_statement_cache(pstmt_cache *cache);
#endif

static sqlite3_stmt * prepared_sql_create_metadata;
static sqlite3_stmt * prepared_sql_create_functions;
static sqlite3_stmt * prepared_sql_create_calls;
static sqlite3_stmt * prepared_sql_create_arguments;
static sqlite3_stmt * prepared_sql_create_promises;
static sqlite3_stmt * prepared_sql_create_associations;
static sqlite3_stmt * prepared_sql_create_evaluations;
static sqlite3_stmt * prepared_sql_create_returns;
static sqlite3_stmt * prepared_sql_create_lifecycle;
static sqlite3_stmt * prepared_sql_create_trigger;
static sqlite3_stmt * prepared_sql_create_distribution;

void compile_prepared_sql_schema_statements() {
    prepared_sql_pragma_asynchronous =
            compile_sql_statement(
                    make_pragma_statement("synchronous", "off"));

    prepared_sql_create_metadata =
            compile_sql_statement(
                    make_create_metadata_statement());
    prepared_sql_create_functions =
            compile_sql_statement(
                    make_create_functions_statement());
    prepared_sql_create_calls =
            compile_sql_statement(
                    make_create_calls_statement());
    prepared_sql_create_arguments =
            compile_sql_statement(
                    make_create_arguments_statement());
    prepared_sql_create_promises =
            compile_sql_statement(
                    make_create_promises_statement());
    prepared_sql_create_associations =
            compile_sql_statement(
                    make_create_promise_associations_statement());
    prepared_sql_create_evaluations =
            compile_sql_statement(
                    make_create_promise_evaluations_statement());
    prepared_sql_create_returns =
            compile_sql_statement(
                    make_create_promise_returns_statement());
    prepared_sql_create_lifecycle =
            compile_sql_statement(
                    make_create_promise_lifecycle_statement());
    prepared_sql_create_trigger =
            compile_sql_statement(
                    make_create_gc_trigger_statement());
    prepared_sql_create_distribution =
      compile_sql_statement(
                    make_create_type_distribution_statement());
}

void compile_prepared_sql_statements() {
    prepared_sql_insert_metadata =
            compile_sql_statement(make_insert_matadata_statement("?","?"));

    prepared_sql_insert_function =
            compile_sql_statement(make_insert_function_statement("?","?","?","?","?"));
    prepared_sql_insert_call =
            compile_sql_statement(make_insert_function_call_statement("?","?","?","?","?","?","?","?","?"));
    prepared_sql_insert_promise =
            compile_sql_statement(make_insert_promise_statement("?", "?", "?", "?", "?", "?", "?"));
    prepared_sql_insert_promise_eval =
            compile_sql_statement(make_insert_promise_evaluation_statement("?","?","?","?","?","?","?","?","?","?","?","?"));
    prepared_sql_insert_promise_return =
            compile_sql_statement(make_insert_promise_return_statement("?","?","?"));
    prepared_sql_insert_promise_lifecycle =
            compile_sql_statement(make_insert_promise_lifecycle_statement("?", "?", "?"));
    prepared_sql_insert_gc_trigger =
            compile_sql_statement(make_insert_gc_trigger_statement("?", "?", "?"));
    prepared_sql_insert_type_distribution =
            compile_sql_statement(make_insert_type_distribution_statement("?", "?", "?", "?"));

    prepared_sql_transaction_begin =
            compile_sql_statement(make_begin_transaction_statement());
    prepared_sql_transaction_commit =
            compile_sql_statement(make_commit_transaction_statement());
    prepared_sql_transaction_abort =
            compile_sql_statement(make_abort_transaction_statement());

    max_function_id_query =
            compile_sql_statement(make_select_max_function_id_statement());
    max_call_id_query =
            compile_sql_statement(make_select_max_call_id_statement());
    max_promise_id_query =
            compile_sql_statement(make_select_max_promise_id_statement());
    min_promise_id_query =
            compile_sql_statement(make_select_min_promise_id_statement());
    max_argument_id_query =
            compile_sql_statement(make_select_max_argument_id_statement());
    max_clock_query =
            compile_sql_statement(make_select_max_promise_evaluation_clock_statement());
    functions_query =
            compile_sql_statement(make_select_all_function_ids_and_definitions_statement());
    arguments_query =
            compile_sql_statement(make_select_all_argument_ids_names_and_function_allegiances_statement());
    already_inserted_functions_query =
            compile_sql_statement(make_select_all_function_ids_statement());
    already_inserted_negative_promises_query =
            compile_sql_statement(make_select_all_negative_promise_ids_statement());
}

// Functions for populating prepared statements.

sqlite3_stmt * populate_metadata_statement(const string key, const string value) {
    if (key.empty())
        sqlite3_bind_null(prepared_sql_insert_metadata, 1);
    else
        sqlite3_bind_text(prepared_sql_insert_metadata, 1, key.c_str(), -1, SQLITE_TRANSIENT);

    if (value.empty())
        sqlite3_bind_null(prepared_sql_insert_metadata, 2);
    else
        sqlite3_bind_text(prepared_sql_insert_metadata, 2, value.c_str(), -1, SQLITE_TRANSIENT);

    return prepared_sql_insert_metadata;
}

sqlite3_stmt * populate_function_statement(const call_info_t & info) {
    sqlite3_bind_int(prepared_sql_insert_function, 1, info.fn_id);

    if (info.loc.empty())
        sqlite3_bind_null(prepared_sql_insert_function, 2);
    else
        sqlite3_bind_text(prepared_sql_insert_function, 2, info.loc.c_str(), -1, SQLITE_TRANSIENT);

    if (info.fn_definition.empty())
        sqlite3_bind_null(prepared_sql_insert_function, 3);
    else
        sqlite3_bind_text(prepared_sql_insert_function, 3, info.fn_definition.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_bind_int(prepared_sql_insert_function, 4, tools::enum_cast(info.fn_type));

    sqlite3_bind_int(prepared_sql_insert_function, 5, info.fn_compiled ? 1 : 0);

    return prepared_sql_insert_function;
}

sqlite3_stmt * populate_arguments_statement(const closure_info_t & info) {
    assert(info.arguments.size() > 0);

    sqlite3_stmt *prepared_statement = get_prepared_sql_insert_argument(info.arguments.size());

    int index = 0;

    for (auto arg_ref : info.arguments.all()) {
        const arg_t & argument = arg_ref.get();
        int offset = index * 4;

        //cerr << get<1>(argument) << " " <<  get<0>(argument) << " " << index << " " << info.call_id << "\n";

        sqlite3_bind_int(prepared_statement, offset + 1, get<1>(argument));
        sqlite3_bind_text(prepared_statement, offset + 2, get<0>(argument).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(prepared_statement, offset + 3, index); // FIXME broken or unnecessary (pick one)
        sqlite3_bind_int(prepared_statement, offset + 4, info.call_id);

        // Rprintf("binding %i %i: %i\n", index, offset + 1, get<1>(argument));
        // Rprintf("binding %i %i: %s\n", index, offset + 2, get<0>(argument).c_str());
        // Rprintf("binding %i %i: %i\n", index, offset + 3, index);
        // Rprintf("binding %i %i: %i\n", index, offset + 4, function_id);

        index++;
    }

    return prepared_statement;
}

sqlite3_stmt * populate_call_statement(const call_info_t & info) {
    sqlite3_bind_int(prepared_sql_insert_call, 1, (int)info.call_id);
    if (info.name.empty())
        sqlite3_bind_null(prepared_sql_insert_call, 2);
    else
        sqlite3_bind_text(prepared_sql_insert_call, 2, info.name.c_str(), -1, SQLITE_TRANSIENT);

    if (info.callsite.empty())
        sqlite3_bind_null(prepared_sql_insert_call, 3);
    else
        sqlite3_bind_text(prepared_sql_insert_call, 3, info.callsite.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_bind_int(prepared_sql_insert_call, 4, info.fn_compiled ? 1 : 0);
    sqlite3_bind_int(prepared_sql_insert_call, 5, (int)info.fn_id);
    sqlite3_bind_int(prepared_sql_insert_call, 6, (int)info.parent_call_id);
    sqlite3_bind_int(prepared_sql_insert_call, 7, (int)info.in_prom_id);
    sqlite3_bind_int(prepared_sql_insert_call, 8, tools::enum_cast(info.parent_on_stack.type));

    switch (info.parent_on_stack.type) {
        case stack_type::NONE:
            sqlite3_bind_null(prepared_sql_insert_call, 9);
            break;
        case stack_type::CALL:
            sqlite3_bind_int(prepared_sql_insert_call, 9, (int) info.parent_on_stack.call_id);
            break;
        case stack_type::PROMISE:
            sqlite3_bind_int(prepared_sql_insert_call, 9, (int) info.parent_on_stack.promise_id);
            break;
    }

    return prepared_sql_insert_call;
}

sqlite3_stmt * populate_promise_statement(const prom_basic_info_t info) {
    sqlite3_bind_int(prepared_sql_insert_promise, 1, (int) info.prom_id);
    sqlite3_bind_int(prepared_sql_insert_promise, 2, tools::enum_cast(info.prom_type));

    if (info.full_type.empty()) {
        sqlite3_bind_null(prepared_sql_insert_promise, 3);
    } else {
        string full_type = full_sexp_type_to_number_string(info.full_type);
        sqlite3_bind_text(prepared_sql_insert_promise, 3, full_type.c_str(), -1, SQLITE_TRANSIENT);
    }

    sqlite3_bind_int(prepared_sql_insert_promise, 4, info.in_prom_id);
    sqlite3_bind_int(prepared_sql_insert_promise, 5, tools::enum_cast(info.parent_on_stack.type));

    switch (info.parent_on_stack.type) {
        case stack_type::NONE:
            sqlite3_bind_null(prepared_sql_insert_promise, 6);
            break;
        case stack_type::CALL:
            sqlite3_bind_int(prepared_sql_insert_promise, 6, (int) info.parent_on_stack.call_id);
            break;
        case stack_type::PROMISE:
            sqlite3_bind_int(prepared_sql_insert_promise, 6, (int) info.parent_on_stack.promise_id);
            break;
    }

    sqlite3_bind_int(prepared_sql_insert_promise, 7, info.depth);

    return prepared_sql_insert_promise;
}

sqlite3_stmt * populate_promise_association_statement(const closure_info_t & info) {
    int num_of_arguments = info.arguments.size();
    assert(num_of_arguments > 0);

    sqlite3_stmt * prepared_statement = get_prepared_sql_insert_promise_assoc(num_of_arguments);

    int index = 0;
    for (auto argument_ref : info.arguments.all()) {
        const arg_t &argument = argument_ref.get();
        arg_id_t arg_id = get<1>(argument);
        prom_id_t promise = get<2>(argument);
        int offset = index * 3;

        sqlite3_bind_int(prepared_statement, offset + 1, promise);
        sqlite3_bind_int(prepared_statement, offset + 2, info.call_id);
        sqlite3_bind_int(prepared_statement, offset + 3, arg_id);

        index++;
    }

    return prepared_statement;
}

sqlite3_stmt * populate_promise_evaluation_statement(prom_eval_t type, const prom_info_t & info) {
    sqlite3_bind_int(prepared_sql_insert_promise_eval, 1, STATE(clock_id++));
    sqlite3_bind_int(prepared_sql_insert_promise_eval, 2, type);
    sqlite3_bind_int(prepared_sql_insert_promise_eval, 3, info.prom_id);
    sqlite3_bind_int(prepared_sql_insert_promise_eval, 4, info.from_call_id);
    sqlite3_bind_int(prepared_sql_insert_promise_eval, 5, info.in_call_id);
    sqlite3_bind_int(prepared_sql_insert_promise_eval, 6, info.in_prom_id);
    sqlite3_bind_int(prepared_sql_insert_promise_eval, 7, tools::enum_cast(info.lifestyle));
    sqlite3_bind_int(prepared_sql_insert_promise_eval, 8, info.effective_distance_from_origin);
    sqlite3_bind_int(prepared_sql_insert_promise_eval, 9, info.actual_distance_from_origin);
    sqlite3_bind_int(prepared_sql_insert_promise_eval, 10, tools::enum_cast(info.parent_on_stack.type));

    switch (info.parent_on_stack.type) {
        case stack_type::NONE:
            sqlite3_bind_null(prepared_sql_insert_promise_eval, 11);
            break;
        case stack_type::CALL:
            sqlite3_bind_int(prepared_sql_insert_promise_eval, 11, (int) info.parent_on_stack.call_id);
            break;
        case stack_type::PROMISE:
            sqlite3_bind_int(prepared_sql_insert_promise_eval, 11, (int) info.parent_on_stack.promise_id);
            break;
    }

    sqlite3_bind_int(prepared_sql_insert_promise_eval, 12, info.depth);

    // in_call_id = current call
    // from_call_id = parent call, for which the promise was created

    return prepared_sql_insert_promise_eval;
}

sqlite3_stmt * populate_promise_return_statement(const prom_info_t & info) {
    sqlite3_bind_int(prepared_sql_insert_promise_return, 1, tools::enum_cast(info.return_type));
    sqlite3_bind_int(prepared_sql_insert_promise_return, 2, info.prom_id);
    sqlite3_bind_int(prepared_sql_insert_promise_return, 3, STATE(clock_id));
    return prepared_sql_insert_promise_return;
}

sqlite3_stmt *populate_promise_lifecycle_statement(const prom_gc_info_t & info) {
    sqlite3_bind_int(prepared_sql_insert_promise_lifecycle, 1, info.promise_id);
    sqlite3_bind_int(prepared_sql_insert_promise_lifecycle, 2, info.event);
    sqlite3_bind_int(prepared_sql_insert_promise_lifecycle, 3, info.gc_trigger_counter);
    return prepared_sql_insert_promise_lifecycle;
}

sqlite3_stmt *populate_gc_trigger_statement(const gc_info_t & info) {
    sqlite3_bind_int(prepared_sql_insert_gc_trigger, 1, info.counter);
    sqlite3_bind_double(prepared_sql_insert_gc_trigger, 2, info.ncells);
    sqlite3_bind_double(prepared_sql_insert_gc_trigger, 3, info.vcells);
    return prepared_sql_insert_gc_trigger;
}

sqlite3_stmt *populate_type_distribution_statement(const type_gc_info_t & info) {
    sqlite3_bind_int(prepared_sql_insert_type_distribution, 1, info.gc_trigger_counter);
    sqlite3_bind_int(prepared_sql_insert_type_distribution, 2, info.type);
    sqlite3_bind_int64(prepared_sql_insert_type_distribution, 3, info.length);
    sqlite3_bind_int64(prepared_sql_insert_type_distribution, 4, info.bytes);
    return prepared_sql_insert_type_distribution;
}

// Functions connecting to the outside world, create SQL and multiplex output.

void psql_recorder_t::function_entry(const closure_info_t & info) {
#ifdef RDT_SQLITE_SUPPORT
    bool align_statements = tracer_conf.pretty_print;
    bool need_to_insert = register_inserted_function(info.fn_id);

    if (need_to_insert) {
        sqlite3_stmt * statement = populate_function_statement(info);
        multiplexer::output(
                multiplexer::payload_t(statement),
                tracer_conf.outputs);
    }

    if (info.arguments.size() > 0) {
        sqlite3_stmt * statement = populate_arguments_statement(info);
        multiplexer::output(
                multiplexer::payload_t(statement),
                tracer_conf.outputs);
    }

    {
        sqlite3_stmt * statement = populate_call_statement(info);
        multiplexer::output(
                multiplexer::payload_t(statement),
                tracer_conf.outputs);
    }

    if (info.arguments.size() > 0) {
        sqlite3_stmt * statement = populate_promise_association_statement(info);
        multiplexer::output(
                multiplexer::payload_t(statement),
                tracer_conf.outputs);
    }
#else
    // FIXME
#endif
}

void psql_recorder_t::builtin_entry(const builtin_info_t & info) {
#ifdef RDT_SQLITE_SUPPORT
    bool need_to_insert = register_inserted_function(info.fn_id);

    if (need_to_insert) {
        sqlite3_stmt * statement = populate_function_statement(info);
        multiplexer::output(
                multiplexer::payload_t(statement),
                tracer_conf.outputs);
    }

    /* always */ {
        sqlite3_stmt * statement = populate_call_statement(info);
        multiplexer::output(
                multiplexer::payload_t(statement),
                tracer_conf.outputs);
    }

    // We do not handle arguments for built-ins.
#else
    // FIXME
#endif
}

void psql_recorder_t::force_promise_entry(const prom_info_t & info) {
#ifdef RDT_SQLITE_SUPPORT
    if (info.prom_id < 0) // if this is a promise from the outside
        if (!negative_promise_already_inserted(info.prom_id)) {
            sqlite3_stmt *statement = populate_promise_statement(info);
            multiplexer::output(
                    multiplexer::payload_t(statement),
                    tracer_conf.outputs);
        }

    /* always */ {
        sqlite3_stmt *statement = populate_promise_evaluation_statement(RDT_SQL_FORCE_PROMISE, info);
        multiplexer::output(
                multiplexer::payload_t(statement),
                tracer_conf.outputs);
    }
#else
    // FIXME
#endif
}

void psql_recorder_t::force_promise_exit(const prom_info_t & info) {
#ifdef RDT_SQLITE_SUPPORT
    /* always */ {
        sqlite3_stmt *statement = populate_promise_return_statement(info);
        multiplexer::output(
                multiplexer::payload_t(statement),
                tracer_conf.outputs);
    }
#else
    // FIXME
#endif
}

void psql_recorder_t::promise_created(const prom_basic_info_t & info) {
#ifdef RDT_SQLITE_SUPPORT
    sqlite3_stmt *statement = populate_promise_statement(info);
    multiplexer::output(
            multiplexer::payload_t(statement),
            tracer_conf.outputs);
#else
    // FIXME
#endif
}

void psql_recorder_t::promise_lookup(const prom_info_t & info) {
#ifdef RDT_SQLITE_SUPPORT
    sqlite3_stmt *statement = populate_promise_evaluation_statement(RDT_SQL_LOOKUP_PROMISE, info);
    multiplexer::output(
            multiplexer::payload_t(statement),
            tracer_conf.outputs);
#else
    // FIXME
#endif
}

void psql_recorder_t::promise_lifecycle(const prom_gc_info_t & info) {
#ifdef RDT_SQLITE_SUPPORT
    sqlite3_stmt *statement = populate_promise_lifecycle_statement(info);
    multiplexer::output(
            multiplexer::payload_t(statement),
            tracer_conf.outputs);
#else
  // FIXME
#endif
}

void psql_recorder_t::gc_exit(const gc_info_t & info) {
#ifdef RDT_SQLITE_SUPPORT
    sqlite3_stmt *statement = populate_gc_trigger_statement(info);
    multiplexer::output(
            multiplexer::payload_t(statement),
            tracer_conf.outputs);
#else
    // FIXME
#endif
}

void psql_recorder_t::vector_alloc(const type_gc_info_t & info) {
#ifdef RDT_SQLITE_SUPPORT
    sqlite3_stmt *statement = populate_type_distribution_statement(info);
    multiplexer::output(
            multiplexer::payload_t(statement),
            tracer_conf.outputs);
#else
  // FIXME
#endif
}

void psql_recorder_t::init_recorder() {
#ifdef RDT_SQLITE_SUPPORT

#else
    // FIXME
#endif
}

void psql_recorder_t::start_trace(const metadata_t & info) {
#ifdef RDT_SQLITE_SUPPORT
    multiplexer::init(tracer_conf.outputs, tracer_conf.filename, tracer_conf.overwrite);

    compile_prepared_sql_schema_statements();

    if (tracer_conf.include_configuration) {
        if (tracer_conf.overwrite) {
            multiplexer::output(
                    multiplexer::payload_t(prepared_sql_pragma_asynchronous),
                    tracer_conf.outputs);

            multiplexer::output(
                    multiplexer::payload_t(prepared_sql_create_metadata),
                    tracer_conf.outputs);

            multiplexer::output(
                    multiplexer::payload_t(prepared_sql_create_functions),
                    tracer_conf.outputs);

            multiplexer::output(
                    multiplexer::payload_t(prepared_sql_create_calls),
                    tracer_conf.outputs);

            multiplexer::output(
                    multiplexer::payload_t(prepared_sql_create_arguments),
                    tracer_conf.outputs);

            multiplexer::output(
                    multiplexer::payload_t(prepared_sql_create_promises),
                    tracer_conf.outputs);

            multiplexer::output(
                    multiplexer::payload_t(prepared_sql_create_associations),
                    tracer_conf.outputs);

            multiplexer::output(
                    multiplexer::payload_t(prepared_sql_create_evaluations),
                    tracer_conf.outputs);

            multiplexer::output(
                    multiplexer::payload_t(prepared_sql_create_returns),
                    tracer_conf.outputs);

            multiplexer::output(
                    multiplexer::payload_t(prepared_sql_create_lifecycle),
                    tracer_conf.outputs);

            multiplexer::output(
                    multiplexer::payload_t(prepared_sql_create_trigger),
                    tracer_conf.outputs);

            multiplexer::output(
                    multiplexer::payload_t(prepared_sql_create_distribution),
                    tracer_conf.outputs);
        }
    }

    compile_prepared_sql_statements();

    for(auto const & i : info) {
        sqlite3_stmt *statement = populate_metadata_statement(i.first, i.second);
        multiplexer::output(
                multiplexer::payload_t(statement),
                tracer_conf.outputs);
    }

    if (!tracer_conf.overwrite && tracer_conf.reload_state) {
        cerr << "doing this~~~~~~~~~~\n";

        multiplexer::int_result max_function_id;
        multiplexer::int_result max_call_id;
        multiplexer::int_result max_clock;
        multiplexer::int_result max_promise_id;
        multiplexer::int_result min_promise_id;
        multiplexer::int_result max_argument_id;
        multiplexer::string_to_int_map_result functions;
        multiplexer::ulong_string_to_ulong_map_result  arguments;

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
        multiplexer::output(
                multiplexer::payload_t(prepared_sql_transaction_begin),
                tracer_conf.outputs);
    }
#else
    // FIXME
#endif
}

void psql_recorder_t::finish_trace(const metadata_t & info) {
#ifdef RDT_SQLITE_SUPPORT
    for(auto const & i : info) {
        sqlite3_stmt *statement = populate_metadata_statement(i.first, i.second);
        multiplexer::output(
                multiplexer::payload_t(statement),
                tracer_conf.outputs);
    }

    multiplexer::output(
            multiplexer::payload_t(prepared_sql_transaction_commit),
            tracer_conf.outputs);

    free_prepared_sql_statements();

    multiplexer::close(tracer_conf.outputs);
#else
    // FIXME
#endif
}

#ifdef RDT_SQLITE_SUPPORT
sqlite3_stmt * compile_sql_statement(sql_stmt_t statement) {
    sqlite3_stmt *prepared_statement;
    int outcome = sqlite3_prepare_v2(multiplexer::sqlite_database,
                                     statement.c_str(), -1, &prepared_statement, NULL);
    if (outcome != SQLITE_OK) {
        fprintf(stderr, "Error: could not compile prepared statement \"%s\", message (%i): %s\n",
                statement.c_str(),
                outcome,
                sqlite3_errmsg(multiplexer::sqlite_database));

        return nullptr;
    }

    return prepared_statement;
}

void free_prepared_sql_statement_cache(pstmt_cache & cache) {
    for(auto const &entry : cache)
        sqlite3_finalize(entry.second);
    cache.clear();
}

void free_prepared_sql_statement_vector(vector<sqlite3_stmt *> & list) {
    for(auto const &statement : list)
        sqlite3_finalize(statement);
    list.clear();
}

void free_prepared_sql_statements() {
    sqlite3_finalize(prepared_sql_insert_function);
    sqlite3_finalize(prepared_sql_insert_call);
    sqlite3_finalize(prepared_sql_insert_promise);
    sqlite3_finalize(prepared_sql_insert_promise_eval);
    sqlite3_finalize(prepared_sql_insert_promise_return);

    sqlite3_finalize(prepared_sql_transaction_begin);
    sqlite3_finalize(prepared_sql_transaction_commit);
    sqlite3_finalize(prepared_sql_transaction_abort);

    sqlite3_finalize(prepared_sql_pragma_asynchronous);

    sqlite3_finalize(max_function_id_query);
    sqlite3_finalize(max_call_id_query);
    sqlite3_finalize(max_promise_id_query);
    sqlite3_finalize(min_promise_id_query);
    sqlite3_finalize(max_argument_id_query);
    sqlite3_finalize(max_clock_query);
    sqlite3_finalize(functions_query);
    sqlite3_finalize(arguments_query);
    sqlite3_finalize(already_inserted_functions_query);
    sqlite3_finalize(already_inserted_negative_promises_query);

    sqlite3_finalize(prepared_sql_create_functions);
    sqlite3_finalize(prepared_sql_create_calls);
    sqlite3_finalize(prepared_sql_create_arguments);
    sqlite3_finalize(prepared_sql_create_promises);
    sqlite3_finalize(prepared_sql_create_associations);
    sqlite3_finalize(prepared_sql_create_evaluations);
    sqlite3_finalize(prepared_sql_create_lifecycle);
    sqlite3_finalize(prepared_sql_create_trigger);
    sqlite3_finalize(prepared_sql_create_distribution);

//    free_prepared_sql_statement_vector(prepared_sql_create_tables_and_views);

    free_prepared_sql_statement_cache(prepared_sql_insert_promise_assocs);
    free_prepared_sql_statement_cache(prepared_sql_insert_arguments);
}

sqlite3_stmt * get_prepared_sql_insert_argument(int values) {
    if (prepared_sql_insert_arguments.count(values))
        return prepared_sql_insert_arguments[values];

    vector<sql_val_cell_t> arguments;
    for (int i = 0 ; i < values; i++)
        arguments.push_back(join({"?", "?", "?", "?"}));

    sql_stmt_t statement = make_insert_arguments_statement(arguments, false);
    sqlite3_stmt *prepared_statement = compile_sql_statement(statement);
    prepared_sql_insert_arguments[values] = prepared_statement;

    return prepared_statement;
}

sqlite3_stmt * get_prepared_sql_insert_promise_assoc(int values) {
    if (prepared_sql_insert_promise_assocs.count(values))
        return prepared_sql_insert_promise_assocs[values];

    vector<sql_val_cell_t> associations;
    for (int i = 0 ; i < values; i++)
        associations.push_back(join({"?", "?", "?"}));

    sql_stmt_t statement = make_insert_promise_associations_statement(associations, false);
    sqlite3_stmt *prepared_statement = compile_sql_statement(statement);
    prepared_sql_insert_promise_assocs[values] = prepared_statement;

    return prepared_statement;
}
#endif
