#include "SqlSerializer.hpp"

SqlSerializer::SqlSerializer(const std::string &database_filepath,
                             const std::string &schema_filepath, bool verbose)
    : verbose(verbose) {
    open_database(database_filepath);
    create_tables(schema_filepath);
    prepare_statements();
}

void SqlSerializer::open_database(const std::string database_path) {
    // Open DB connection.
    int outcome = sqlite3_open(database_path.c_str(), &database);
    if (outcome != SQLITE_OK) {
        cerr << "Error: could not open DB connection to filename "
             << database_path << ", "
             << " message (" << outcome
             << "): " << string(sqlite3_errmsg(database)) << "\n";
        exit(1);
    }
}

void SqlSerializer::create_tables(const std::string schema_path) {
    execute(compile("pragma synchronous = off;"));
    execute(compile(get_file_contents(schema_path.c_str())));
}

SqlSerializer::~SqlSerializer() {
    close_database();
    finalize_statements();
}

void SqlSerializer::close_database() { sqlite3_close(database); }

void SqlSerializer::finalize_statements() {
    sqlite3_finalize(insert_metadata_statement);
    sqlite3_finalize(insert_function_statement);
    sqlite3_finalize(insert_argument_statement);
    sqlite3_finalize(insert_call_statement);
    sqlite3_finalize(insert_promise_statement);
    sqlite3_finalize(insert_promise_association_statement);
    sqlite3_finalize(insert_promise_evaluation_statement);
    sqlite3_finalize(insert_promise_return_statement);
    sqlite3_finalize(insert_promise_lifecycle_statement);
    sqlite3_finalize(insert_gc_trigger_statement);
    sqlite3_finalize(insert_type_distribution_statement);
}

void SqlSerializer::prepare_statements() {
    insert_metadata_statement = compile("insert into metadata values (?,?);");

    insert_function_statement =
        compile("insert into functions values (?,?,?,?,?);");

    insert_call_statement =
        compile("insert into calls values (?,?,?,?,?,?,?,?,?);");

    insert_argument_statement =
        compile("insert into arguments values (?,?,?,?);");

    insert_promise_statement =
        compile("insert into promises values (?,?,?,?,?,?,?);");

    insert_promise_association_statement =
        compile("insert into promise_associations values (?,?,?);");

    insert_promise_evaluation_statement = compile(
        "insert into promise_evaluations values (?,?,?,?,?,?,?,?,?,?,?,?);");

    insert_promise_return_statement =
        compile("insert into promise_returns values (?,?,?);");

    insert_promise_lifecycle_statement =
        compile("insert into promise_lifecycle values (?,?,?);");

    insert_gc_trigger_statement =
        compile("insert into gc_trigger values (?,?,?);");

    insert_type_distribution_statement =
        compile("insert into type_distribution values (?,?,?,?);");
}

void SqlSerializer::serialize_start_trace(const metadata_t &info) {

    for (auto const &i : info) {
        execute(populate_metadata_statement(i.first, i.second));
    }

    execute(compile("begin transaction;"));
}

void SqlSerializer::serialize_finish_trace(const metadata_t &info) {

    for (auto const &i : info) {
        execute(populate_metadata_statement(i.first, i.second));
    }
    execute(compile("commit;"));
}

sqlite3_stmt *SqlSerializer::compile(const char *statement) {
    sqlite3_stmt *prepared_statement;
    int outcome =
        sqlite3_prepare_v2(database, statement, -1, &prepared_statement, NULL);
    if (outcome != SQLITE_OK) {
        fprintf(stderr, "Error: could not compile prepared statement \"%s\", "
                        "message (%i): %s\n",
                statement, outcome, sqlite3_errmsg(database));

        exit(1);
    }

    return prepared_statement;
}

void SqlSerializer::execute(sqlite3_stmt *statement) {
    int outcome = sqlite3_step(statement);
    if (outcome != SQLITE_DONE) {
        cerr << "Error: could not execute prepared statement \""
             << sqlite3_sql(statement) << "\", "
             << "message (" << outcome << "): " << sqlite3_errmsg(database)
             << "\n";
    }
    sqlite3_reset(statement);
}

void SqlSerializer::serialize_promise_lifecycle(const prom_gc_info_t &info) {
    sqlite3_bind_int(insert_promise_lifecycle_statement, 1, info.promise_id);
    sqlite3_bind_int(insert_promise_lifecycle_statement, 2, info.event);
    sqlite3_bind_int(insert_promise_lifecycle_statement, 3,
                     info.gc_trigger_counter);
    execute(insert_promise_lifecycle_statement);
}

void SqlSerializer::serialize_gc_exit(const gc_info_t &info) {
    sqlite3_bind_int(insert_gc_trigger_statement, 1, info.counter);
    sqlite3_bind_double(insert_gc_trigger_statement, 2, info.ncells);
    sqlite3_bind_double(insert_gc_trigger_statement, 3, info.vcells);
    execute(insert_gc_trigger_statement);
}

void SqlSerializer::serialize_vector_alloc(const type_gc_info_t &info) {
    sqlite3_bind_int(insert_type_distribution_statement, 1,
                     info.gc_trigger_counter);
    sqlite3_bind_int(insert_type_distribution_statement, 2, info.type);
    sqlite3_bind_int64(insert_type_distribution_statement, 3, info.length);
    sqlite3_bind_int64(insert_type_distribution_statement, 4, info.bytes);
    execute(insert_type_distribution_statement);
}

void SqlSerializer::serialize_promise_expression_lookup(
    const prom_info_t &info) {
    execute(populate_promise_evaluation_statement(
        info, RDT_SQL_LOOKUP_PROMISE_EXPRESSION));
}

void SqlSerializer::serialize_promise_lookup(const prom_info_t &info) {
    execute(
        populate_promise_evaluation_statement(info, RDT_SQL_LOOKUP_PROMISE));
}

// TODO - can type be included in the info struct itself ?
// TODO - better way for state access ?
sqlite3_stmt *
SqlSerializer::populate_promise_evaluation_statement(const prom_info_t &info,
                                                     const int type) {

    sqlite3_bind_int(insert_promise_evaluation_statement, 1,
                     tracer_state().clock_id++);
    sqlite3_bind_int(insert_promise_evaluation_statement, 2, type);
    sqlite3_bind_int(insert_promise_evaluation_statement, 3, info.prom_id);
    sqlite3_bind_int(insert_promise_evaluation_statement, 4, info.from_call_id);
    sqlite3_bind_int(insert_promise_evaluation_statement, 5, info.in_call_id);
    sqlite3_bind_int(insert_promise_evaluation_statement, 6, info.in_prom_id);
    sqlite3_bind_int(insert_promise_evaluation_statement, 7,
                     to_underlying_type(info.lifestyle));
    sqlite3_bind_int(insert_promise_evaluation_statement, 8,
                     info.effective_distance_from_origin);
    sqlite3_bind_int(insert_promise_evaluation_statement, 9,
                     info.actual_distance_from_origin);
    sqlite3_bind_int(insert_promise_evaluation_statement, 10,
                     to_underlying_type(info.parent_on_stack.type));

    switch (info.parent_on_stack.type) {
        case stack_type::NONE:
            sqlite3_bind_null(insert_promise_evaluation_statement, 11);
            break;
        case stack_type::CALL:
            sqlite3_bind_int(insert_promise_evaluation_statement, 11,
                             (int)info.parent_on_stack.call_id);
            break;
        case stack_type::PROMISE:
            sqlite3_bind_int(insert_promise_evaluation_statement, 11,
                             (int)info.parent_on_stack.promise_id);
            break;
    }

    sqlite3_bind_int(insert_promise_evaluation_statement, 12, info.depth);

    // in_call_id = current call
    // from_call_id = parent call, for which the promise was created
    return insert_promise_evaluation_statement;
}

void SqlSerializer::serialize_function_entry(const closure_info_t &info) {

    bool need_to_insert = register_inserted_function(info.fn_id);

    if (need_to_insert) {
        execute(populate_function_statement(info));
    }

    for (int index = 0; index < info.arguments.size(); ++index) {
        execute(populate_insert_argument_statement(info, index));
    }

    execute(populate_call_statement(info));

    for (int index = 0; index < info.arguments.size(); ++index) {
        execute(populate_promise_association_statement(info, index));
    }
}

void SqlSerializer::serialize_builtin_entry(const builtin_info_t &info) {
    bool need_to_insert = register_inserted_function(info.fn_id);

    if (need_to_insert) {
        execute(populate_function_statement(info));
    }

    /* always */ { execute(populate_call_statement(info)); }
}

void SqlSerializer::serialize_force_promise_entry(const prom_info_t &info) {
    if (info.prom_id < 0) // if this is a promise from the outside
        if (!negative_promise_already_inserted(info.prom_id)) {
            execute(populate_insert_promise_statement(info));
        }

    execute(populate_promise_evaluation_statement(info, RDT_SQL_FORCE_PROMISE));
}

void SqlSerializer::serialize_force_promise_exit(const prom_info_t &info) {
    sqlite3_bind_int(insert_promise_return_statement, 1,
                     to_underlying_type(info.return_type));
    sqlite3_bind_int(insert_promise_return_statement, 2, info.prom_id);
    sqlite3_bind_int(insert_promise_return_statement, 3,
                     tracer_state().clock_id);
    execute(insert_promise_return_statement);
}

void SqlSerializer::serialize_promise_created(const prom_basic_info_t &info) {
    execute(populate_insert_promise_statement(info));
}

sqlite3_stmt *SqlSerializer::populate_insert_promise_statement(
    const prom_basic_info_t &info) {
    sqlite3_bind_int(insert_promise_statement, 1, (int)info.prom_id);
    sqlite3_bind_int(insert_promise_statement, 2,
                     to_underlying_type(info.prom_type));

    if (info.full_type.empty()) {
        sqlite3_bind_null(insert_promise_statement, 3);
    } else {
        string full_type = full_sexp_type_to_number_string(info.full_type);
        sqlite3_bind_text(insert_promise_statement, 3, full_type.c_str(), -1,
                          SQLITE_TRANSIENT);
    }

    sqlite3_bind_int(insert_promise_statement, 4, info.in_prom_id);
    sqlite3_bind_int(insert_promise_statement, 5,
                     to_underlying_type(info.parent_on_stack.type));

    switch (info.parent_on_stack.type) {
        case stack_type::NONE:
            sqlite3_bind_null(insert_promise_statement, 6);
            break;
        case stack_type::CALL:
            sqlite3_bind_int(insert_promise_statement, 6,
                             (int)info.parent_on_stack.call_id);
            break;
        case stack_type::PROMISE:
            sqlite3_bind_int(insert_promise_statement, 6,
                             (int)info.parent_on_stack.promise_id);
            break;
    }
    sqlite3_bind_int(insert_promise_statement, 7, info.depth);
    return insert_promise_statement;
}

sqlite3_stmt *SqlSerializer::populate_call_statement(const call_info_t &info) {
    sqlite3_bind_int(insert_call_statement, 1, (int)info.call_id);
    if (info.name.empty())
        sqlite3_bind_null(insert_call_statement, 2);
    else
        sqlite3_bind_text(insert_call_statement, 2, info.name.c_str(), -1,
                          SQLITE_TRANSIENT);

    if (info.callsite.empty())
        sqlite3_bind_null(insert_call_statement, 3);
    else
        sqlite3_bind_text(insert_call_statement, 3, info.callsite.c_str(), -1,
                          SQLITE_TRANSIENT);

    sqlite3_bind_int(insert_call_statement, 4, info.fn_compiled ? 1 : 0);
    sqlite3_bind_int(insert_call_statement, 5, (int)info.fn_id);
    sqlite3_bind_int(insert_call_statement, 6, (int)info.parent_call_id);
    sqlite3_bind_int(insert_call_statement, 7, (int)info.in_prom_id);
    sqlite3_bind_int(insert_call_statement, 8,
                     to_underlying_type(info.parent_on_stack.type));

    switch (info.parent_on_stack.type) {
        case stack_type::NONE:
            sqlite3_bind_null(insert_call_statement, 9);
            break;
        case stack_type::CALL:
            sqlite3_bind_int(insert_call_statement, 9,
                             (int)info.parent_on_stack.call_id);
            break;
        case stack_type::PROMISE:
            sqlite3_bind_int(insert_call_statement, 9,
                             (int)info.parent_on_stack.promise_id);
            break;
    }

    return insert_call_statement;
}

sqlite3_stmt *SqlSerializer::populate_promise_association_statement(
    const closure_info_t &info, int index) {

    const arg_t &argument = info.arguments.all()[index].get();
    arg_id_t arg_id = get<1>(argument);
    prom_id_t promise = get<2>(argument);

    sqlite3_bind_int(insert_promise_association_statement, 1, promise);
    sqlite3_bind_int(insert_promise_association_statement, 2, info.call_id);
    sqlite3_bind_int(insert_promise_association_statement, 3, arg_id);

    return insert_promise_association_statement;
}

sqlite3_stmt *SqlSerializer::populate_metadata_statement(const string key,
                                                         const string value) {
    if (key.empty())
        sqlite3_bind_null(insert_metadata_statement, 1);
    else
        sqlite3_bind_text(insert_metadata_statement, 1, key.c_str(), -1,
                          SQLITE_TRANSIENT);

    if (value.empty())
        sqlite3_bind_null(insert_metadata_statement, 2);
    else
        sqlite3_bind_text(insert_metadata_statement, 2, value.c_str(), -1,
                          SQLITE_TRANSIENT);

    return insert_metadata_statement;
}

sqlite3_stmt *
SqlSerializer::populate_function_statement(const call_info_t &info) {
    sqlite3_bind_int(insert_function_statement, 1, info.fn_id);

    if (info.loc.empty())
        sqlite3_bind_null(insert_function_statement, 2);
    else
        sqlite3_bind_text(insert_function_statement, 2, info.loc.c_str(), -1,
                          SQLITE_TRANSIENT);

    if (info.fn_definition.empty())
        sqlite3_bind_null(insert_function_statement, 3);
    else
        sqlite3_bind_text(insert_function_statement, 3,
                          info.fn_definition.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_bind_int(insert_function_statement, 4,
                     to_underlying_type(info.fn_type));

    sqlite3_bind_int(insert_function_statement, 5, info.fn_compiled ? 1 : 0);

    return insert_function_statement;
}

sqlite3_stmt *
SqlSerializer::populate_insert_argument_statement(const closure_info_t &info,
                                                  int index) {
    const arg_t &argument = info.arguments.all()[index].get();
    sqlite3_bind_int(insert_argument_statement, 1, get<1>(argument));
    sqlite3_bind_text(insert_argument_statement, 2, get<0>(argument).c_str(),
                      -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(insert_argument_statement, 3,
                     index); // FIXME broken or unnecessary (pick one)
    sqlite3_bind_int(insert_argument_statement, 4, info.call_id);
    return insert_argument_statement;
}
