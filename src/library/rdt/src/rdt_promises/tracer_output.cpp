//
// Created by nohajc on 27.3.17.
//

#include <string>
#include <sstream>
#include <fstream>

#include "tracer_output.h"
#include "tracer_conf.h"
#include "tracer_state.h"

#include "../rdt.h"

using namespace std;

FILE *output = NULL;

void rdt_print(OutputFormat string_format, std::initializer_list<string> strings) {
    if (tracer_conf.output_format != RDT_OUTPUT_BOTH && string_format != tracer_conf.output_format)
        return;

    switch (tracer_conf.output_type) {
        case RDT_FILE:
            for (auto string : strings)
                fprintf(output, "%s", string.c_str());
            break;
        case RDT_R_PRINT_AND_SQLITE:
        case RDT_SQLITE: {
            // If R is compiled without RDT_SQLITE_SUPPORT then this will print a warning and print out SQL to
            // Rprintf instead.
#ifdef RDT_SQLITE_SUPPORT
            char *error_msg = NULL;
            stringstream sql;
            for (auto string : strings)
                sql << string;

            string sql_string = sql.str();

            if (tracer_conf.output_type == RDT_R_PRINT_AND_SQLITE)
                Rprintf("%s", sql_string.c_str());

            int outcome = sqlite3_exec(sqlite_database, sql_string.c_str(), NULL, 0, &error_msg);

            if (outcome != SQLITE_OK) {
                fprintf(stderr, "SQLite: [%i] %s/%s in: %s\n", outcome, error_msg, sqlite3_errmsg(sqlite_database), sql_string.c_str());
                if (error_msg)
                    sqlite3_free(error_msg);
            }

            break;
#else
            fprintf(stderr, "-- SQLite support is missing, printing SQL to console:\n");
#endif
        } case RDT_R_PRINT:
            for (auto string : strings)
                Rprintf(string.c_str());
            break;
    }
}

string print_unwind(const char *type, call_id_t call_id) {
    stringstream stream;
    prepend_prefix(&stream);
    stream << type << " "
           << "unwind(" << (tracer_conf.call_id_use_ptr_fmt ? hex : dec) << call_id << ")\n";
    return stream.str();
}


string print_builtin(const char *type, const char *loc, const char *name, fn_addr_t id, call_id_t call_id) {
    stringstream stream;
    prepend_prefix(&stream);

    stream << type << " "
           << "loc(" << CHKSTR(loc) << ") ";
    stream << "call(";
    if (tracer_conf.call_id_use_ptr_fmt) {
        stream << "0x" << hex << call_id;
    }
    else {
        stream << call_id;
    }

    stream << ") fun(" << CHKSTR(name) << "=0x" << hex << id << ")\n";

    return stream.str();
}

string print_promise(const char *type, const char *loc, const char *name, prom_id_t id, call_id_t in_call_id, call_id_t from_call_id) {
    stringstream stream;
    prepend_prefix(&stream);

    auto num_fmt = tracer_conf.call_id_use_ptr_fmt ? hex : dec;
    string num_pref = tracer_conf.call_id_use_ptr_fmt ? "0x" : "";

    stream << type << " "
           << "loc(" << CHKSTR(loc) << ") "
           << "prom(" << CHKSTR(name) << "=" << id << ") ";
    stream << "in(" << num_pref << num_fmt << in_call_id << ") ";
    stream << "from(" << num_pref << num_fmt << from_call_id << ")\n";

    return stream.str();
}

void prepend_prefix(stringstream *stream) {
    if (tracer_conf.output_format == RDT_OUTPUT_TRACE) {
        if (tracer_conf.pretty_print)
            (*stream) << string(STATE(indent), ' ');
    } else
        (*stream) << "-- ";
}

string print_function(const char *type, const char *loc, const char *name, fn_addr_t function_id, call_id_t call_id, arglist_t const& arguments) {
    stringstream stream;
    prepend_prefix(&stream);

    stream << type << " "
           << "loc(" << CHKSTR(loc) << ") ";

    stream << "call(";
    if (tracer_conf.call_id_use_ptr_fmt) {
        stream << "0x" << hex << call_id;
    }
    else {
        stream << call_id;
    }

    stream << ") fun(" << CHKSTR(name) << "=0x" << hex << function_id << ") ";

    // print argument names and the promises bound to them
    stream << "arguments(";
    int i = 0;
    for (auto arg_ref : arguments.all()) {
        const arg_t & argument = arg_ref.get();
        prom_id_t promise = get<2>(argument);
        //fprintf(output, "%s=%#x", get<0>(a).c_str(), p[0]);
        stream << get<0>(argument).c_str() << "=" << promise;

        if (i < arguments.size() - 1)
            stream << ",";
        //fprintf(output, ";"); // semicolon separates individual args
        ++i;
    }

    stream << ")\n";
    //fprintf(output, ")\n");

    return stream.str();
}

/*
 * ===========================================================
 *                            S Q L
 * ===========================================================
 */

#ifdef RDT_SQLITE_SUPPORT
static std::string RDT_SQLITE_SCHEMA = "src/library/rdt/sql/schema.sql";
sqlite3 *sqlite_database;

// Prepared statement objects.
static sqlite3_stmt *prepared_sql_insert_function = nullptr;
static sqlite3_stmt *prepared_sql_insert_call = nullptr;
static sqlite3_stmt *prepared_sql_insert_promise = nullptr;
static sqlite3_stmt *prepared_sql_insert_promise_eval = nullptr;
static sqlite3_stmt *prepared_sql_transaction_begin = nullptr;
static sqlite3_stmt *prepared_sql_transaction_commit = nullptr;
static sqlite3_stmt *prepared_sql_transaction_abort = nullptr;
#endif


#ifdef RDT_SQLITE_SUPPORT
typedef map<int, sqlite3_stmt*> pstmt_cache;

pstmt_cache prepared_sql_insert_promise_assocs;
pstmt_cache prepared_sql_insert_arguments;


// Internal functions
static sqlite3_stmt *get_prepared_sql_insert_statement(string database_table, int num_columns, int num_values, pstmt_cache *cache) {
    sqlite3_stmt *statement;// = prepared_sql_insert_functions[num_values];

    // Statement already exists
    if (cache->count(num_values))
        return (*cache)[num_values];

    // Prepare the "?, ?, ... ?" string for inserts
    string value_cell;
    value_cell.reserve(1 + (num_columns - 1) * 3);
    value_cell += "?";
    for (int i = 1; i < num_columns; i++)
        value_cell += ", ?";

    // Special case for 1 value.
    if (num_values == 1) {
        string statement_sql = ("insert into " + database_table + " values (" + value_cell + " );");

        int result = sqlite3_prepare_v2(sqlite_database, statement_sql.c_str(), -1, &statement, NULL);

        if (result != SQLITE_OK) {
            fprintf(stderr, "SQLite cannot prepare statement '%s' error: [%i] %s\n",
                    statement_sql.c_str(), result, sqlite3_errmsg(sqlite_database));
            return nullptr;
        }

        (*cache)[1] = statement;
        return statement;
    }

    // General case;
    stringstream statement_sql;
    statement_sql << "insert into ";
    statement_sql << database_table;
    statement_sql << " select ";
    statement_sql << value_cell;
    for (int i = num_values-1; i--; num_values>=0) {
        statement_sql << "union all select ";
        statement_sql << value_cell;
    }
    statement_sql << ";";

    int result = sqlite3_prepare_v2(sqlite_database, statement_sql.str().c_str(), -1, &statement, NULL);
    if (result != SQLITE_OK) {
        fprintf(stderr, "SQLite cannot prepare statement '%s' error: [%i] %s\n",
                statement_sql.str().c_str(), result, sqlite3_errmsg(sqlite_database));
        return nullptr;
    }

    (*cache)[num_values] = statement;
    return statement;
}

static sqlite3_stmt *get_prepared_sql_insert_promise_assoc(int num_values) {
    return get_prepared_sql_insert_statement("promise_associations", 3, num_values, &prepared_sql_insert_promise_assocs);
}

static sqlite3_stmt *get_prepared_sql_insert_argument(int num_values) {
    return get_prepared_sql_insert_statement("arguments", 4, num_values, &prepared_sql_insert_arguments);
}



static void compile_prepared_sql_statements() {
    int result;
    result = sqlite3_prepare_v2(sqlite_database,
                       "insert into functions values (?, ?, ?);",
                       -1, &prepared_sql_insert_function, NULL);

    if (result != SQLITE_OK)
        fprintf(stderr, "SQLite cannot prepare statement: [%i] %s\n", result, sqlite3_errmsg(sqlite_database));


    if (result != SQLITE_OK)
        fprintf(stderr, "SQLite cannot prepare statement: [%i] %s\n", result, sqlite3_errmsg(sqlite_database));

    result = sqlite3_prepare_v2(sqlite_database,
                       "insert into calls values (?, ?, ?, ?, ?, ?);",
                       -1, &prepared_sql_insert_call, NULL);

    if (result != SQLITE_OK)
        fprintf(stderr, "SQLite cannot prepare statement: [%i] %s\n", result, sqlite3_errmsg(sqlite_database));

    result = sqlite3_prepare_v2(sqlite_database,
                       "insert into promises values (?);",
                       -1, &prepared_sql_insert_promise, NULL);

    if (result != SQLITE_OK)
        fprintf(stderr, "SQLite cannot prepare statement: [%i] %s\n", result, sqlite3_errmsg(sqlite_database));


    if (result != SQLITE_OK)
        fprintf(stderr, "SQLite cannot prepare statement: [%i] %s\n", result, sqlite3_errmsg(sqlite_database));

    result = sqlite3_prepare_v2(sqlite_database,
                       "insert into promise_evaluations values (?, ?, ?, ?);",
                       -1, &prepared_sql_insert_promise_eval, NULL);

    if (result != SQLITE_OK)
        fprintf(stderr, "SQLite cannot prepare statement: [%i] %s\n", result, sqlite3_errmsg(sqlite_database));

    result = sqlite3_prepare_v2(sqlite_database,
                                "begin transaction;",
                                -1, &prepared_sql_transaction_begin, NULL);

    if (result != SQLITE_OK)
        fprintf(stderr, "SQLite cannot prepare statement: [%i] %s\n", result, sqlite3_errmsg(sqlite_database));

    result = sqlite3_prepare_v2(sqlite_database,
                                "commit;",
                                -1, &prepared_sql_transaction_commit, NULL);

    if (result != SQLITE_OK)
        fprintf(stderr, "SQLite cannot prepare statement: [%i] %s\n", result, sqlite3_errmsg(sqlite_database));

    result = sqlite3_prepare_v2(sqlite_database,
                                "rollback;",
                                -1, &prepared_sql_transaction_abort, NULL);

    if (result != SQLITE_OK)
        fprintf(stderr, "SQLite cannot prepare statement: [%i] %s\n", result, sqlite3_errmsg(sqlite_database));

}

static void free_prepared_sql_statement_cache(pstmt_cache *cache) {
    for(auto const &entry : (*cache))
        sqlite3_finalize(entry.second);
    cache->clear();
}

static void free_prepared_sql_statements() {
    sqlite3_finalize(prepared_sql_insert_function);
    sqlite3_finalize(prepared_sql_insert_call);
    sqlite3_finalize(prepared_sql_insert_promise);
    sqlite3_finalize(prepared_sql_insert_promise_eval);
    sqlite3_finalize(prepared_sql_transaction_begin);
    sqlite3_finalize(prepared_sql_transaction_commit);
    sqlite3_finalize(prepared_sql_transaction_abort);

    free_prepared_sql_statement_cache(&prepared_sql_insert_promise_assocs);
    free_prepared_sql_statement_cache(&prepared_sql_insert_arguments);
}
#endif

static string wrap_nullable_string(const char* s) {
    return s == NULL ? "NULL" : "'" + string(s) + "'";
}

static string escape_sql_quote_string(string s) {
    size_t position = 0;
    while ((position = s.find("'", position)) != string::npos) {
        s.replace(position, 1, "''");
        position += 2; // length of "''"
    }
    return s;
}

// I'm not using prepared statements, since we have to escape in the case of SQL script files, so: two birds with one stone.
static string wrap_and_escape_nullable_string(const char* s) {
    return s == NULL ? "NULL" : "'" +
                                escape_sql_quote_string(string(s)) + "'";
}

static string wrap_string(const string & s) {
    return "'" + s + "'";
}

static bool execute_prepared_sql_statement(sqlite3_stmt *statement) {

    int result = sqlite3_step(statement);
    sqlite3_reset(statement);

    if (result != SQLITE_DONE) {
        fprintf(stderr, "SQLite cannot execute statement '%s' error: [%i] %s\n",
                sqlite3_sql(statement), result, sqlite3_errmsg(sqlite_database));
        return false;
    }

    return true;
}


// Exported functions
void rdt_init_sqlite(const string& filename) {
#ifdef RDT_SQLITE_SUPPORT
    int outcome;
    char *error_msg = NULL;
    outcome = sqlite3_open(filename.c_str(), &sqlite_database);

    if (outcome) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(sqlite_database));
        return;
    } else {
        fprintf(stderr, "Opening database: %s\n", filename);
    }

    ifstream schema_file(RDT_SQLITE_SCHEMA);
    string schema_string;
    schema_file.seekg(0, ios::end);
    schema_string.reserve(schema_file.tellg());
    schema_file.seekg(0, ios::beg);
    schema_string.assign((istreambuf_iterator<char>(schema_file)), istreambuf_iterator<char>());

    //if (output_type == RDT_R_PRINT_AND_SQLITE || output_type == RDT_R)
    //Rprintf("%s\n", schema_string.c_str());

    outcome = sqlite3_exec(sqlite_database, schema_string.c_str(), NULL, 0, &error_msg);

    if (outcome != SQLITE_OK) {
        fprintf(stderr, "SQLite: [%i] %s/%s in: \"%s\"\n", outcome, error_msg, sqlite3_errmsg(sqlite_database), schema_string.c_str());
        sqlite3_free(error_msg);
    }

    compile_prepared_sql_statements();
#else
    fprintf(stderr, "-- SQLite support is missing...?\n");
#endif
}

void rdt_configure_sqlite() {
#ifdef RDT_SQLITE_SUPPORT
    //PRAGMA synchronous = OFF and PRAGMA journal_mode = MEMORY
    string config_query = "pragma synchronous = OFF;\n";
    if (tracer_conf.output_format == RDT_OUTPUT_SQL)
        rdt_print(RDT_OUTPUT_SQL, {config_query});
    else
        rdt_print(RDT_OUTPUT_COMPILED_SQLITE, {config_query});

#endif
}

void rdt_close_sqlite() {
#ifdef RDT_SQLITE_SUPPORT
    free_prepared_sql_statements();
    sqlite3_close(sqlite_database);
#endif
}


void run_prep_sql_function(fn_addr_t function_id, arglist_t const& arguments, const char* location, const char* definition) {
    // Don't run anything if one was previously generated.
    if(STATE(already_inserted_functions).count(function_id))
        return;

    // Bind parameters to statement.
    sqlite3_bind_int(prepared_sql_insert_function, 1, function_id);

    if (location == NULL)
        sqlite3_bind_null(prepared_sql_insert_function, 2);
    else
        sqlite3_bind_text(prepared_sql_insert_function, 2, location, -1, SQLITE_STATIC);

    if (definition == NULL)
        sqlite3_bind_null(prepared_sql_insert_function, 3);
    else
        sqlite3_bind_text(prepared_sql_insert_function, 3, definition, -1, SQLITE_STATIC);

    //Rprintf("function: %s\n", sqlite3_sql(prepared_sql_insert_function));

    // Insert function into DB using prepared statement.
    auto result = sqlite3_step(prepared_sql_insert_function);

    if (result != SQLITE_DONE) {
        Rprintf("Error executing compiled SQL expression: [%s] %s\n", "function", sqlite3_errmsg(sqlite_database));
        return;
    }

    sqlite3_reset(prepared_sql_insert_function);

    int num_of_arguments = arguments.size();
    if (num_of_arguments == 0)
        return;

    sqlite3_stmt *prepared_statement = get_prepared_sql_insert_argument(num_of_arguments);
    int index = 0;

    for (auto arg_ref : arguments.all()) {
        const arg_t & argument = arg_ref.get();
        int offset = index * 4;

        sqlite3_bind_int(prepared_statement, offset + 1, get<1>(argument));
        sqlite3_bind_text(prepared_statement, offset + 2, get<0>(argument).c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(prepared_statement, offset + 3, index);
        sqlite3_bind_int(prepared_statement, offset + 4, function_id);

//            Rprintf("binding %i %i: %i\n", index, offset + 1, get<1>(argument));
//            Rprintf("binding %i %i: %s\n", index, offset + 2, get<0>(argument).c_str());
//            Rprintf("binding %i %i: %i\n", index, offset + 3, index);
//            Rprintf("binding %i %i: %i\n", index, offset + 4, function_id);

        index++;
    }

    execute_prepared_sql_statement(prepared_statement);
    STATE(already_inserted_functions).insert(function_id);
}

string mk_sql_function(fn_addr_t function_id, arglist_t const& arguments, const char* location, const char* definition) {
    stringstream stream;
    // Don't generate anything if one was previously generated.
    if(STATE(already_inserted_functions).count(function_id))
        return "";

    // Generate `functions' update containing function definition.
    stream << "insert into functions values ("
           << "0x" << hex << function_id << ",";
    stream << wrap_nullable_string(location) << ","
           << wrap_and_escape_nullable_string(definition) <<
           ");\n";

    // Generate `arguments' update wrt function above.
    if (arguments.size() > 0) {
        stream << "insert into arguments select";

        int index = 0;
        for (auto arg_ref : arguments.all()) {
            const arg_t & argument = arg_ref.get();
            if (index)
                stream << (tracer_conf.pretty_print ? "\n            " : " ") << "union all select";

            stream << " "
                   << dec << get<1>(argument) << ","
                   << wrap_string(get<0>(argument)) << ","
                   << index << ","
                   << "0x" << hex << function_id;

            index++;
        }
        stream << ";\n";
    }
    STATE(already_inserted_functions).insert(function_id);

    return stream.str();
}

void run_prep_sql_function_call(call_id_t call_id, env_addr_t call_ptr, const char *name, const char* location, int call_type, fn_addr_t function_id) {
    sqlite3_bind_int(prepared_sql_insert_call, 1, (int)call_id);
    sqlite3_bind_int(prepared_sql_insert_call, 2, (int)call_ptr);

    if (name == NULL)
        sqlite3_bind_null(prepared_sql_insert_call, 3);
    else
        sqlite3_bind_text(prepared_sql_insert_call, 3, name, -1, SQLITE_STATIC);

    if (location == NULL)
        sqlite3_bind_null(prepared_sql_insert_call, 4);
    else
        sqlite3_bind_text(prepared_sql_insert_call, 4, location, -1, SQLITE_STATIC);

    sqlite3_bind_int(prepared_sql_insert_call, 5, call_type);
    sqlite3_bind_int(prepared_sql_insert_call, 6, (int)function_id);

    //Rprintf("call: %s\n", sqlite3_sql(prepared_sql_insert_call));

    // Insert function into DB using prepared statement.
    auto result = sqlite3_step(prepared_sql_insert_call);

    if (result != SQLITE_DONE) {
        Rprintf("Error executing compiled SQL expression: [%s] %s\n", "call", sqlite3_errmsg(sqlite_database));
        return;
    }

    sqlite3_reset(prepared_sql_insert_call);
}

string mk_sql_function_call(call_id_t call_id, env_addr_t call_ptr, const char *name, const char* location, int call_type, fn_addr_t function_id) {
    std::stringstream stream;
    stream << "insert into calls values ("
           << dec << call_id << ","
           << "0x" << hex << call_ptr << ","
           << wrap_nullable_string(name) << ","
           << wrap_nullable_string(location) << ","
           << dec << call_type << ","
           << "0x" << hex << function_id
           << ");\n";
    return stream.str();
}

void run_prep_sql_promise(prom_id_t prom_id) {
    std::stringstream promise_stream;

    sqlite3_bind_int(prepared_sql_insert_promise, 1, prom_id);

    auto result = sqlite3_step(prepared_sql_insert_promise);

    //Rprintf("promise: %s\n", sqlite3_sql(prepared_sql_insert_promise));

    if (result != SQLITE_DONE) {
        Rprintf("Error executing compiled SQL expression: [%s] %s\n", "promise", sqlite3_errmsg(sqlite_database));
        return;
    }

    sqlite3_reset(prepared_sql_insert_promise);
}

string mk_sql_promise(prom_id_t prom_id) {
    std::stringstream promise_stream;

    promise_stream << "insert into promises values (";
    promise_stream << prom_id << ");\n";

    return promise_stream.str();
}

void run_prep_sql_promise_assoc(arglist_t const& arguments, call_id_t call_id) {
    int num_of_arguments = arguments.size();
    if (num_of_arguments == 0)
        return;

    sqlite3_stmt *prepared_statement = get_prepared_sql_insert_promise_assoc(num_of_arguments);
    int index = 0;

    for (auto arg_ref : arguments.all()) {
        const arg_t &argument = arg_ref.get();
        arg_id_t arg_id = get<1>(argument);
        prom_id_t promise = get<2>(argument);
        int offset = index * 3;

        sqlite3_bind_int(prepared_statement, offset + 1, promise);
        sqlite3_bind_int(prepared_statement, offset + 2, call_id);
        sqlite3_bind_int(prepared_statement, offset + 3, arg_id);

        index++;
    }
    execute_prepared_sql_statement(prepared_statement);
}

string mk_sql_promise_assoc(arglist_t const& arguments, call_id_t call_id) {
    std::stringstream promise_association_stream;

    if (arguments.size() > 0) {
        promise_association_stream << "insert into promise_associations select";
        int index = 0;
        for (auto arg_ref : arguments.all()) {
            const arg_t & argument = arg_ref.get();
            arg_id_t arg_id = get<1>(argument);
            prom_id_t promise = get<2>(argument);

            if (index)
                promise_association_stream << (tracer_conf.pretty_print ? "\n            " : " ")
                                           << "union all select";

            promise_association_stream << " "
                                       << promise << ","
                                       << dec << call_id << ","
                                       << dec << arg_id;

            index++;
        }
        promise_association_stream << ";\n";
    }

    return promise_association_stream.str();
}


void run_prep_sql_promise_evaluation(int event_type, prom_id_t promise_id, call_id_t call_id) {
    sqlite3_bind_int(prepared_sql_insert_promise_eval, 1, STATE(clock_id));
    sqlite3_bind_int(prepared_sql_insert_promise_eval, 2, event_type);
    sqlite3_bind_int(prepared_sql_insert_promise_eval, 3, promise_id);
    sqlite3_bind_int(prepared_sql_insert_promise_eval, 4, call_id);

    STATE(clock_id)++;

    //Rprintf("promise eval: %s %d %d %d\n", sqlite3_sql(prepared_sql_insert_promise_eval), event_type, promise_id, call_id);

    auto result = sqlite3_step(prepared_sql_insert_promise_eval);

    if (result != SQLITE_DONE) {
        Rprintf("Error executing compiled SQL expression: [%s] %s\n", "promise eval", sqlite3_errmsg(sqlite_database));
        return;
    }

    sqlite3_reset(prepared_sql_insert_promise_eval);
}


string mk_sql_promise_evaluation(int event_type, prom_id_t promise_id, call_id_t call_id) {
    std::stringstream stream;
    stream << "insert into promise_evaluations values ("
           << "$next_id,"
           << "0x" << hex << event_type << ",";
    stream << dec << promise_id << ",";
    stream << dec << call_id
           << ");\n";
    return stream.str();
}


void rdt_begin_transaction() {
    if (tracer_conf.output_format == RDT_OUTPUT_COMPILED_SQLITE && tracer_conf.output_type == RDT_SQLITE) {
        int result = sqlite3_step(prepared_sql_transaction_begin);
        if (result != SQLITE_DONE) {
            Rprintf("Error executing compiled SQL expression: [%s] %s\n", "begin transaction", sqlite3_errmsg(sqlite_database));
            return;
        }
        sqlite3_reset(prepared_sql_transaction_begin);
    } else {
        rdt_print(RDT_OUTPUT_SQL, {"begin transaction;\n"});
    }
}

void rdt_commit_transaction() {
    if (tracer_conf.output_format == RDT_OUTPUT_COMPILED_SQLITE && tracer_conf.output_type == RDT_SQLITE) {
        int result = sqlite3_step(prepared_sql_transaction_commit);
        if (result != SQLITE_DONE) {
            Rprintf("Error executing compiled SQL expression: [%s] %s\n", "commit transaction", sqlite3_errmsg(sqlite_database));
            return;
        }
        sqlite3_reset(prepared_sql_transaction_commit);
    } else {
        rdt_print(RDT_OUTPUT_SQL, {"commit;\n"});
    }
}

void rdt_abort_transaction() {
    if (tracer_conf.output_format == RDT_OUTPUT_COMPILED_SQLITE && tracer_conf.output_type == RDT_SQLITE) {
        int result = sqlite3_step(prepared_sql_transaction_abort);
        if (result != SQLITE_DONE) {
            Rprintf("Error executing compiled SQL expression: [%s] %s\n", "abort transaction", sqlite3_errmsg(sqlite_database));
            return;
        }
        sqlite3_reset(prepared_sql_transaction_abort);
    } else {
        rdt_print(RDT_OUTPUT_SQL, {"rollback;\n"});
    }
}
