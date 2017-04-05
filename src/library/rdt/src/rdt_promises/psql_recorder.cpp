//
// Created by nohajc on 29.3.17.
//

#include <stdio.h>

#include "psql_recorder.h"
#include "tracer_conf.h"

#include "sql_generator.h"
#include "multiplexer.h"

#ifdef RDT_SQLITE_SUPPORT
#include <sqlite3.h>
#endif

#include <string>

using namespace std;
using namespace sql_generator;

typedef int prom_eval_t;

#ifdef RDT_SQLITE_SUPPORT
// Helper functions.
sqlite3_stmt* compile_sql_statement(sql_stmt_t statement);

// Prepared statement objects.
static sqlite3_stmt * prepared_sql_insert_function = nullptr;
static sqlite3_stmt * prepared_sql_insert_call = nullptr;
static sqlite3_stmt * prepared_sql_insert_promise = nullptr;
static sqlite3_stmt * prepared_sql_insert_promise_eval = nullptr;
static sqlite3_stmt * prepared_sql_transaction_begin = nullptr;
static sqlite3_stmt * prepared_sql_transaction_commit = nullptr;
static sqlite3_stmt * prepared_sql_transaction_abort = nullptr;
static sqlite3_stmt * prepared_sql_pragma_asynchronous = nullptr;

static vector<sqlite3_stmt *> prepared_sql_create_tables_and_views;
// FIXME load schema statement

// Prepared statement object getters for objects with variable arrity.
sqlite3_stmt *get_prepared_sql_insert_argument(int);
sqlite3_stmt *get_prepared_sql_insert_promise_assoc(int);

typedef map<int, sqlite3_stmt *> pstmt_cache;
pstmt_cache prepared_sql_insert_promise_assocs;
pstmt_cache prepared_sql_insert_arguments;

// More helper functions.
void free_prepared_sql_statements();
void free_prepared_sql_statement_cache(pstmt_cache *cache);
//sqlite3_stmt *get_prepared_sql_insert_statement(string table, int columns, int values, pstmt_cache *cache);
#endif

void compile_prepared_sql_schema_statements() {
    prepared_sql_pragma_asynchronous =
            compile_sql_statement(make_pragma_statement("synchronous", "off"));

    // We must split the schema into separate statements;
    for (sql_stmt_t statement : split_into_individual_statements(make_create_tables_and_views_statement()))
        prepared_sql_create_tables_and_views.push_back(compile_sql_statement(statement));
}

void compile_prepared_sql_statements() {
    prepared_sql_insert_function =
            compile_sql_statement(make_insert_function_statement("?","?","?"));
    prepared_sql_insert_call =
            compile_sql_statement(make_insert_function_call_statement("?","?","?","?","?","?"));
    prepared_sql_insert_promise =
            compile_sql_statement(make_insert_promise_statement("?"));
    prepared_sql_insert_promise_eval =
            compile_sql_statement(make_insert_promise_evaluation_statement("?","?","?","?"));

    prepared_sql_transaction_begin =
            compile_sql_statement(make_begin_transaction_statement());
    prepared_sql_transaction_commit =
            compile_sql_statement(make_commit_transaction_statement());
    prepared_sql_transaction_abort =
            compile_sql_statement(make_abort_transaction_statement());
}

// Functions for populating prepared statements.

sqlite3_stmt * populate_function_statement(const call_info_t & info) {
    sqlite3_bind_int(prepared_sql_insert_function, 1, info.fn_id);

    if (info.loc.empty())
        sqlite3_bind_null(prepared_sql_insert_function, 2);
    else
        sqlite3_bind_text(prepared_sql_insert_function, 2, info.loc.c_str(), -1, SQLITE_STATIC);

    if (info.fn_definition.empty())
        sqlite3_bind_null(prepared_sql_insert_function, 3);
    else
        sqlite3_bind_text(prepared_sql_insert_function, 3, info.fn_definition.c_str(), -1, SQLITE_STATIC);

    return prepared_sql_insert_function;
}

sqlite3_stmt * populate_arguments_statement(const call_info_t & info) {
    assert(info.arguments.size() > 0);

    sqlite3_stmt *prepared_statement = get_prepared_sql_insert_argument(info.arguments.size());

    int index = 0;

    for (auto arg_ref : info.arguments.all()) {
        const arg_t & argument = arg_ref.get();
        int offset = index * 4;

        sqlite3_bind_int(prepared_statement, offset + 1, get<1>(argument));
        sqlite3_bind_text(prepared_statement, offset + 2, get<0>(argument).c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(prepared_statement, offset + 3, index);
        sqlite3_bind_int(prepared_statement, offset + 4, info.fn_id);

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
    sqlite3_bind_int(prepared_sql_insert_call, 2, (int)info.call_ptr); // FIXME do we really need this?

    if (info.fqfn.empty())
        sqlite3_bind_null(prepared_sql_insert_call, 3);
    else
        sqlite3_bind_text(prepared_sql_insert_call, 3, info.fqfn.c_str(), -1, SQLITE_STATIC);

    if (info.loc.empty())
        sqlite3_bind_null(prepared_sql_insert_call, 4);
    else
        sqlite3_bind_text(prepared_sql_insert_call, 4, info.loc.c_str(), -1, SQLITE_STATIC);

    sqlite3_bind_int(prepared_sql_insert_call, 5, info.call_type);
    sqlite3_bind_int(prepared_sql_insert_call, 6, (int)info.fn_id);

    return prepared_sql_insert_call;
}

sqlite3_stmt * populate_promise_statement(const prom_id_t id) {
    sqlite3_bind_int(prepared_sql_insert_promise, 1, (int)id);
    return prepared_sql_insert_promise;
}

sqlite3_stmt * populate_promise_association_statement(const call_info_t & info) {
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

    STATE(clock_id)++;

    // in_call_id = current call
    // from_call_id = TODO what is it

    return prepared_sql_insert_promise_eval;
}

// Functions connecting to the outside world, create SQL and multiplex output.

void psql_recorder_t::function_entry(const call_info_t & info) {
#ifdef RDT_SQLITE_SUPPORT
    bool align_statements = tracer_conf.pretty_print;

    if (STATE(already_inserted_functions).count(info.fn_id) == 0) {
        sqlite3_stmt * statement = populate_function_statement(info);
        multiplexer::output(
                multiplexer::payload_t(statement),
                tracer_conf.outputs);

        STATE(already_inserted_functions).insert(info.fn_id);
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

void psql_recorder_t::builtin_entry(const call_info_t & info) {
#ifdef RDT_SQLITE_SUPPORT
    if (STATE(already_inserted_functions).count(info.fn_id) == 0) {
        sqlite3_stmt * statement = populate_function_statement(info);
        multiplexer::output(
                multiplexer::payload_t(statement),
                tracer_conf.outputs);

        STATE(already_inserted_functions).insert(info.fn_id);
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
    sqlite3_stmt *statement = populate_promise_evaluation_statement(RDT_SQL_FORCE_PROMISE, info);
    multiplexer::output(
            multiplexer::payload_t(statement),
            tracer_conf.outputs);
#else
    // FIXME
#endif
}

void psql_recorder_t::promise_created(const prom_id_t & prom_id) {
#ifdef RDT_SQLITE_SUPPORT
    sqlite3_stmt *statement = populate_promise_statement(prom_id);
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

void psql_recorder_t::init_recorder() {
#ifdef RDT_SQLITE_SUPPORT

#else
    // FIXME
#endif
}

void psql_recorder_t::start_trace() {
#ifdef RDT_SQLITE_SUPPORT
    multiplexer::init(tracer_conf.outputs, tracer_conf.filename, tracer_conf.overwrite);

    if (tracer_conf.include_configuration)
        if (tracer_conf.overwrite) {
            compile_prepared_sql_schema_statements();

            multiplexer::output(
                    multiplexer::payload_t(prepared_sql_pragma_asynchronous),
                    tracer_conf.outputs);

            for (auto statement : prepared_sql_create_tables_and_views) {
                multiplexer::output(
                        multiplexer::payload_t(statement),
                        tracer_conf.outputs);
            }
        }

    if (tracer_conf.overwrite)
        compile_prepared_sql_statements();

    /* always */ {
        multiplexer::output(
                multiplexer::payload_t(prepared_sql_transaction_begin),
                tracer_conf.outputs);
    }

    // TODO cleanup comment
    // if (overwrite or writing to a new db/file)
    //      throw away any precompiled statements
    //      re-compile statements
    //
    // if (output_configuration)
    //      if (overwrite or writing to a new db/file)
    //          load pragmas
    //          load schema
    //
    //
#else
    // FIXME
#endif
}

void psql_recorder_t::finish_trace() {
#ifdef RDT_SQLITE_SUPPORT
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

    sqlite3_finalize(prepared_sql_transaction_begin);
    sqlite3_finalize(prepared_sql_transaction_commit);
    sqlite3_finalize(prepared_sql_transaction_abort);

    sqlite3_finalize(prepared_sql_pragma_asynchronous);

    free_prepared_sql_statement_vector(prepared_sql_create_tables_and_views);

    free_prepared_sql_statement_cache(prepared_sql_insert_promise_assocs);
    free_prepared_sql_statement_cache(prepared_sql_insert_arguments);
}

//pstmt_cache prepared_sql_insert_promise_assocs;
//pstmt_cache prepared_sql_insert_arguments;

sqlite3_stmt * get_prepared_sql_insert_argument(int values) {
    if (prepared_sql_insert_arguments.count(values))
        return prepared_sql_insert_arguments[values];

    vector<sql_val_cell_t> arguments;
    for (int i = 0 ; i < values; i ++)
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
    for (int i = 0 ; i < values; i ++)
        associations.push_back(join({"?", "?", "?"}));

    sql_stmt_t statement = make_insert_promise_associations_statement(associations, false);
    sqlite3_stmt *prepared_statement = compile_sql_statement(statement);
    prepared_sql_insert_arguments[values] = prepared_statement;

    return prepared_statement;
}
#endif