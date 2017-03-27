//#ifdef HAVE_CONFIG_H
//# include <config.h>
//#endif
//#include <Defn.h>

#include <vector>
#include <string>
#include <stack>
#include <sstream>
#include <initializer_list>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <tuple>
#include <map>
#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <string>
#include <fstream>
#include <streambuf>
#include <functional>


//#ifdef SQLITE3_H
#define RDT_SQLITE_SUPPORT
//#endif

#ifdef RDT_SQLITE_SUPPORT
#include <sqlite3.h>
static std::string RDT_SQLITE_SCHEMA = "src/library/rdt/sql/schema.sql";
static sqlite3 *sqlite_database;

// Prepared statement objects.
static sqlite3_stmt *prepared_sql_insert_function = nullptr;
static sqlite3_stmt *prepared_sql_insert_call = nullptr;
static sqlite3_stmt *prepared_sql_insert_promise = nullptr;
static sqlite3_stmt *prepared_sql_insert_promise_eval = nullptr;
static sqlite3_stmt *prepared_sql_transaction_begin = nullptr;
static sqlite3_stmt *prepared_sql_transaction_commit = nullptr;
static sqlite3_stmt *prepared_sql_transaction_abort = nullptr;
#endif

//#include <Defn.h>

//#define RDT_PROMSES_SQL_SUPPORT
//#ifdef RDT_PROMSES_SQL_SUPPORT
//#include <sqlite3.h>
//sqlite3 *db;
//#endif

extern "C" {
#include "../../../main/inspect.h"
#include "r.h"
#include "rdt.h"
}

#include "rdt_register_hook.h"

using namespace std;

// Use generated call IDs instead of function env addresses
#define RDT_CALL_ID

#define RDT_LOOKUP_PROMISE 0x0
#define RDT_FORCE_PROMISE 0xF

typedef uintptr_t rid_t;
typedef intptr_t rsid_t;

typedef rid_t prom_addr_t;
typedef rid_t env_addr_t;
typedef rid_t fn_addr_t;
typedef rsid_t prom_id_t;
typedef rid_t call_id_t;

typedef int arg_id_t;

#define RID_INVALID (rid_t)-1

static FILE *output = NULL;


enum OutputFormat: char {RDT_OUTPUT_TRACE, RDT_OUTPUT_SQL, RDT_OUTPUT_BOTH, RDT_OUTPUT_COMPILED_SQLITE};
enum Output: char {RDT_R_PRINT, RDT_FILE, RDT_SQLITE, RDT_R_PRINT_AND_SQLITE};


template<typename T>
class option {
    bool is_supplied;
    T value;

public:
    option(const T& val) {
        value = val;
        is_supplied = false;
    }

    option& operator=(const T& val) {
        value = val;
        is_supplied = true;
        return *this;
    }

    bool operator==(const option& opt) const {
        return value == opt.value;
    }

    bool operator!=(const option& opt) const {
        return !operator==(opt);
    }

    bool operator==(const T& val) const {
        return value == val;
    }

    bool operator!=(const T& val) const {
        return !operator==(val);
    }

    T& operator*() {
        return value;
    }

    T* operator->() {
        return &value;
    }

    // Implicit cast to T
    operator T() const {
        return value;
    }

    bool supplied() const {
        return is_supplied;
    }
};

struct tracer_conf_t {
    option<string> filename;
    option<int> output_type;
    option<int> output_format;
    option<bool> pretty_print;
    option<int> indent_width;
    option<bool> call_id_use_ptr_fmt;

    bool overwrite;

    tracer_conf_t() :
            // Config defaults
            filename("tracer.db"),
            output_type(RDT_R_PRINT),
            output_format(RDT_OUTPUT_TRACE),
            pretty_print(true),
            overwrite(false),
            indent_width(4),
#ifdef RDT_CALL_ID
            call_id_use_ptr_fmt(false)
#else
            call_id_use_prt_fmt(true)
#endif
            {
//        filename = NULL;
//        output_type = RDT_R_PRINT_AND_SQLITE;
//        output_format = RDT_OUTPUT_BOTH;
//        pretty_print = true;
//        overwrite = false;
//        indent_width = 4;
//#ifdef RDT_CALL_ID
//        call_id_use_ptr_fmt = false;
//#else
//        call_id_use_ptr_fmt = true;
//#endif
//        first_update = true;
    }

    template<typename T>
    static inline bool opt_changed(const T& old_opt, const T& new_opt) {
        return new_opt.supplied() && old_opt != new_opt;
    }

    // Update configuration in a smart way
    // (e.g. ignore changes of output type/format if overwrite == false)
    void update(const tracer_conf_t & conf) {
#define OPT_CHANGED(opt) opt_changed(opt, conf.opt)

        bool conf_changed =
                OPT_CHANGED(filename) ||
                OPT_CHANGED(output_type) ||
                OPT_CHANGED(output_format) ||
                OPT_CHANGED(pretty_print) ||
                OPT_CHANGED(indent_width) ||
                OPT_CHANGED(call_id_use_ptr_fmt);

        if (conf.overwrite || conf_changed) {
            *this = conf; // updates all members
            overwrite = true;
        }
        else {
            overwrite = false;
        }

#undef OPT_CHANGED
    }
};

static tracer_conf_t tracer_conf; // init default configuration

static inline rid_t get_sexp_address(SEXP e) {
    return (rid_t)e;
}

static inline void prepend_prefix(stringstream *stream);
static inline prom_id_t make_promise_id(SEXP promise, bool negative = false);
static inline string mk_sql_promise(prom_id_t prom_id);

static inline string print_unwind(const char *type, call_id_t call_id) {
    stringstream stream;
    prepend_prefix(&stream);
    stream << type << " "
           << "unwind(" << (tracer_conf.call_id_use_ptr_fmt ? hex : dec) << call_id << ")\n";
    return stream.str();
}

static inline void rdt_print(OutputFormat string_format, std::initializer_list<string> strings) {
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

typedef pair<fn_addr_t, string> arg_key_t;

struct tracer_state_t {
    int indent;
    int clock_id;
    // Function call stack (may be useful)
    // Whenever R makes a function call, we generate a function ID and store that ID on top of the stack
    // so that we know where we are (e.g. when printing function ID at function_exit hook)
    stack<call_id_t, vector<call_id_t>> fun_stack; // Should be reset on each tracer pass
#ifdef RDT_CALL_ID
#define CALL_ID_FMT "%d"
    stack<env_addr_t , vector<env_addr_t>> curr_env_stack; // Should be reset on each tracer pass
#else
#define CALL_ID_FMT "%#x"
#endif

    // Map from promise IDs to call IDs
    unordered_map<prom_id_t, call_id_t> promise_origin; // Should be reset on each tracer pass
    unordered_set<prom_id_t> fresh_promises;
    // Map from promise address to promise ID;
    unordered_map<prom_addr_t, prom_id_t> promise_ids;

    call_id_t call_id_counter; // IDs assigned should be globally unique but we can reset it after each pass if overwrite is true)
    prom_id_t prom_id_counter; // IDs assigned should be globally unique but we can reset it after each pass if overwrite is true)
    prom_id_t prom_neg_id_counter;

    unordered_set<fn_addr_t> already_inserted_functions; // Should be kept across Rdt calls (unless overwrite is true)
    arg_id_t argument_id_sequence; // Should be globally unique (can reset between tracer calls if overwrite is true)
    map<arg_key_t, arg_id_t> argument_ids; // Should be kept across Rdt calls (unless overwrite is true)

    void start_pass(const SEXP prom) {
        if (tracer_conf.overwrite) {
            reset();
        }

        indent = 0;
        clock_id = 0;

        // We have to make sure the stack is not empty
        // when referring to the promise created by call to Rdt.
        // This is just a dummy call and environment.
        fun_stack.push(0);
#ifdef RDT_CALL_ID
        curr_env_stack.push(0);
#endif

        prom_addr_t prom_addr = get_sexp_address(prom);
        prom_id_t prom_id = make_promise_id(prom);
        promise_origin[prom_id] = 0;
        rdt_print(RDT_OUTPUT_SQL, {mk_sql_promise(prom_id)});
    }

    void finish_pass() {
        fun_stack.pop();
#ifdef RDT_CALL_ID
        curr_env_stack.pop();
#endif

        promise_origin.clear();
    }

    // When doing longjump (exception thrown, etc.) this function gets the target environment
    // and unwinds function call stack until that environment is on top. It also fixes indentation.
    void adjust_fun_stack(SEXP rho) {
        call_id_t call_id;
        env_addr_t call_addr;

        while (!fun_stack.empty() &&
#ifdef RDT_CALL_ID
            (call_addr = curr_env_stack.top()) && get_sexp_address(rho) != call_addr
#else
            (call_id = fun_stack.top()) && get_sexp_address(rho) != call_id
#endif
                ) {
#ifdef RDT_CALL_ID
            call_id = fun_stack.top();
            curr_env_stack.pop();
#endif
            fun_stack.pop();

            if (tracer_conf.pretty_print)
                indent -= tracer_conf.indent_width;
            rdt_print(RDT_OUTPUT_TRACE, {print_unwind("<=", call_id)});
        }
    }

    static tracer_state_t& get_instance() {
        static tracer_state_t tracer_state;
        return tracer_state;
    }

private:
    tracer_state_t() {
        indent = 0;
        clock_id = 0;
        call_id_counter = 0;
        prom_id_counter = 0;
        prom_neg_id_counter = 0;
        argument_id_sequence = 0;
    }

    void reset() {
	clock_id = 0;
        call_id_counter = 0;
        prom_id_counter = 0;
        prom_neg_id_counter = 0;
        argument_id_sequence = 0;
        already_inserted_functions.clear();
        argument_ids.clear();
        promise_ids.clear();
    }
};

static inline tracer_state_t& tracer_state() {
    return tracer_state_t::get_instance();
}

// Helper macro for accessing state properties
#define STATE(property) tracer_state().property

//typedef vector<rid_t> prom_vec_t;
typedef tuple<string, arg_id_t, prom_id_t> arg_t;
typedef tuple<arg_id_t, prom_id_t> anon_arg_t;

class arglist_t {
    vector<arg_t> args;
    vector<arg_t> ddd_kw_args;
    vector<arg_t> ddd_pos_args;
    mutable vector<reference_wrapper<const arg_t>> arg_refs;
    mutable bool update_arg_refs;

    template<typename T>
    void push_back_tmpl(T&& value, bool ddd) {
        if (ddd) {
            arg_t new_value = value;
            string & arg_name = get<0>(new_value);
            arg_name = "...[" + arg_name + "]";
            ddd_kw_args.push_back(new_value);
        }
        else {
            args.push_back(forward<T>(value));
        }
        update_arg_refs = true;
    }

    template<typename T>
    void push_back_anon_tmpl(T&& value) {
        string arg_name = "...[" + to_string(ddd_pos_args.size()) + "]";
        // prepend string arg_name to anon_arg_t value
        arg_t new_value = tuple_cat(make_tuple(arg_name), value);

        ddd_pos_args.push_back(new_value);
        update_arg_refs = true;
    }
public:
    arglist_t() {
        update_arg_refs = true;
    }

    void push_back(const arg_t& value, bool ddd = false) {
        push_back_tmpl(value, ddd);
    }

    void push_back(arg_t&& value, bool ddd = false) {
        push_back_tmpl(value, ddd);
    }

    void push_back(const anon_arg_t& value) {
        push_back_anon_tmpl(value);
    }

    void push_back(anon_arg_t&& value) {
        push_back_anon_tmpl(value);
    }

    // Return vector of references to elements of our three inner vectors
    // so we can iterate over all of them in one for loop.
    vector<reference_wrapper<const arg_t>> all() const {
        if (update_arg_refs) {
            arg_refs.assign(args.begin(), args.end());
            arg_refs.insert(arg_refs.end(), ddd_kw_args.begin(), ddd_kw_args.end());
            arg_refs.insert(arg_refs.end(), ddd_pos_args.begin(), ddd_pos_args.end());
            update_arg_refs = false;
        }

        return arg_refs;
    }

    size_t size() const {
        if (update_arg_refs) {
            all();
        }
        return arg_refs.size();
    }
};


static inline void prepend_prefix(stringstream *stream) {
    if (tracer_conf.output_format == RDT_OUTPUT_TRACE) {
        if (tracer_conf.pretty_print)
            (*stream) << string(STATE(indent), ' ');
    } else
        (*stream) << "-- ";
}

#ifdef RDT_SQLITE_SUPPORT
typedef map<int, sqlite3_stmt*> pstmt_cache;

static inline sqlite3_stmt *get_prepared_sql_insert_statement(string database_table, int num_columns, int num_values, pstmt_cache *cache) {
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

pstmt_cache prepared_sql_insert_promise_assocs;
pstmt_cache prepared_sql_insert_arguments;

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

static inline void free_prepared_sql_statement_cache(pstmt_cache *cache) {
    for(auto const &entry : (*cache))
        sqlite3_finalize(entry.second);
    cache->clear();
}

static inline void free_prepared_sql_statements() {
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

static inline void rdt_init_sqlite(const string& filename) {
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

static inline void rdt_configure_sqlite() {
#ifdef RDT_SQLITE_SUPPORT
    //PRAGMA synchronous = OFF and PRAGMA journal_mode = MEMORY
    string config_query = "pragma synchronous = OFF;\n";
    if (tracer_conf.output_format == RDT_OUTPUT_SQL)
        rdt_print(RDT_OUTPUT_SQL, {config_query});
    else
        rdt_print(RDT_OUTPUT_COMPILED_SQLITE, {config_query});

#endif
}

static inline void rdt_close_sqlite() {
#ifdef RDT_SQLITE_SUPPORT
    free_prepared_sql_statements();
    sqlite3_close(sqlite_database);
#endif
}

static inline string print_builtin(const char *type, const char *loc, const char *name, fn_addr_t id, call_id_t call_id) {
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

static inline string print_promise(const char *type, const char *loc, const char *name, prom_id_t id, call_id_t in_call_id, call_id_t from_call_id) {
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

static inline string wrap_nullable_string(const char* s) {
    return s == NULL ? "NULL" : "'" + string(s) + "'";
}

static inline string escape_sql_quote_string(string s) {
    size_t position = 0;
    while ((position = s.find("'", position)) != string::npos) {
        s.replace(position, 1, "''");
        position += 2; // length of "''"
    }
    return s;
}

// I'm not using prepared statements, since we have to escape in the case of SQL script files, so: two birds with one stone.
static inline string wrap_and_escape_nullable_string(const char* s) {
    return s == NULL ? "NULL" : "'" +
            escape_sql_quote_string(string(s)) + "'";
}

static inline string wrap_string(const string & s) {
    return "'" + s + "'";
}

static inline bool execute_prepared_sql_statement(sqlite3_stmt *statement) {

    int result = sqlite3_step(statement);
    sqlite3_reset(statement);

    if (result != SQLITE_DONE) {
        fprintf(stderr, "SQLite cannot execute statement '%s' error: [%i] %s\n",
                sqlite3_sql(statement), result, sqlite3_errmsg(sqlite_database));
        return false;
    }

    return true;
}

static inline void run_prep_sql_function(fn_addr_t function_id, arglist_t const& arguments, const char* location, const char* definition) {
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

static inline string mk_sql_function(fn_addr_t function_id, arglist_t const& arguments, const char* location, const char* definition) {
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

static inline void run_prep_sql_function_call(call_id_t call_id, env_addr_t call_ptr, const char *name, const char* location, int call_type, fn_addr_t function_id) {
    sqlite3_bind_int(prepared_sql_insert_call, 1, call_id);
    sqlite3_bind_int(prepared_sql_insert_call, 2, call_ptr);

    if (name == NULL)
        sqlite3_bind_null(prepared_sql_insert_call, 3);
    else
        sqlite3_bind_text(prepared_sql_insert_call, 3, name, -1, SQLITE_STATIC);

    if (location == NULL)
        sqlite3_bind_null(prepared_sql_insert_call, 4);
    else
        sqlite3_bind_text(prepared_sql_insert_call, 4, location, -1, SQLITE_STATIC);

    sqlite3_bind_int(prepared_sql_insert_call, 5, call_type);
    sqlite3_bind_int(prepared_sql_insert_call, 6, function_id);

    //Rprintf("call: %s\n", sqlite3_sql(prepared_sql_insert_call));

    // Insert function into DB using prepared statement.
    auto result = sqlite3_step(prepared_sql_insert_call);

    if (result != SQLITE_DONE) {
        Rprintf("Error executing compiled SQL expression: [%s] %s\n", "call", sqlite3_errmsg(sqlite_database));
        return;
    }

    sqlite3_reset(prepared_sql_insert_call);
}

static inline string mk_sql_function_call(call_id_t call_id, env_addr_t call_ptr, const char *name, const char* location, int call_type, fn_addr_t function_id) {
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

static inline void run_prep_sql_promise(prom_id_t prom_id) {
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

static inline string mk_sql_promise(prom_id_t prom_id) {
    std::stringstream promise_stream;

    promise_stream << "insert into promises values (";
    promise_stream << prom_id << ");\n";

    return promise_stream.str();
}

static inline void run_prep_sql_promise_assoc(arglist_t const& arguments, call_id_t call_id) {
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

static inline string mk_sql_promise_assoc(arglist_t const& arguments, call_id_t call_id) {
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


static inline void run_prep_sql_promise_evaluation(int event_type, prom_id_t promise_id, call_id_t call_id) {
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


static inline string mk_sql_promise_evaluation(int event_type, prom_id_t promise_id, call_id_t call_id) {
    std::stringstream stream;
    stream << "insert into promise_evaluations values ("
           << "$next_id,"
           << "0x" << hex << event_type << ",";
    stream << dec << promise_id << ",";
    stream << dec << call_id
           << ");\n";
    return stream.str();
}


static inline string print_function(const char *type, const char *loc, const char *name, fn_addr_t function_id, call_id_t call_id, arglist_t const& arguments) {
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

static inline void rdt_begin_transaction() {
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

static inline void rdt_commit_transaction() {
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

static inline void rdt_abort_transaction() {
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

static inline int count_elements(SEXP list) {
    int counter = 0;
    SEXP tmp = list;
    for (; tmp != R_NilValue; counter++)
        tmp = CDR(tmp);
    return counter;
}

static inline prom_id_t get_promise_id(SEXP promise) {
    if (promise == R_NilValue)
        return RID_INVALID;
    if (TYPEOF(promise) != PROMSXP)
        return RID_INVALID;

    // A new promise is always created for each argument.
    // Even if the argument is already a promise passed from the caller, it gets re-wrapped.
    prom_addr_t prom_addr = get_sexp_address(promise);
    prom_id_t prom_id;

    auto & promise_ids = STATE(promise_ids);
    auto it = promise_ids.find(prom_addr);
    if (it != promise_ids.end()){
        prom_id = it->second;
    }
    else {
        prom_id = make_promise_id(promise, true);
	// FIXME: prepared SQL statements
        rdt_print(RDT_OUTPUT_SQL, {mk_sql_promise(prom_id)});
    }

    return prom_id;
}

static inline prom_id_t make_promise_id(SEXP promise, bool negative) {
    if (promise == R_NilValue)
        return RID_INVALID;

    prom_addr_t prom_addr = get_sexp_address(promise);
    prom_id_t prom_id;

    if (negative) {
        prom_id = --STATE(prom_neg_id_counter);
    }
    else {
        prom_id = STATE(prom_id_counter)++;
    }
    STATE(promise_ids)[prom_addr] = prom_id;

    return prom_id;
}

static inline fn_addr_t get_function_id(SEXP func) {
    assert(TYPEOF(func) == CLOSXP);
    return get_sexp_address(func);
}

#ifdef RDT_CALL_ID
static inline call_id_t make_funcall_id(SEXP function) {
    if (function == R_NilValue)
        return RID_INVALID;

    return ++STATE(call_id_counter);
}
#else
static inline call_id_t make_funcall_id(SEXP fn_env) {
    assert(fn_env != NULL);
    return get_sexp_address(fn_env);
}
#endif


// Wraper for findVar. Does not look up the value if it already is PROMSXP.
static SEXP get_promise(SEXP var, SEXP rho) {
    SEXP prom = R_NilValue;

    if (TYPEOF(var) == PROMSXP) {
        prom = var;
    } else if (TYPEOF(var) == SYMSXP) {
        prom = findVar(var, rho);
    }

    return prom;
}

// XXX Better way to do this?
//class ArgumentIDKey {
//  public:
//    rid_t function_id;
//    string argument;
//    ArgumentIDKey(rid_t function_id, string argument) {
//      this->function_id = function_id;
//      this->argument = argument;
//    }
//    bool operator<(const ArgumentIDKey& k) const {
//      if (this->function_id == k.function_id)
//        return this->argument.compare(k.argument);
//      return this->function_id < k.function_id;
//    }
//};


static inline arg_id_t get_argument_id(fn_addr_t function_id, const string & argument) {
    arg_key_t key = make_pair(function_id, argument);
    auto iterator = STATE(argument_ids).find(key);

    if (iterator != STATE(argument_ids).end()) {
        return iterator->second;
    }

    arg_id_t argument_id = ++STATE(argument_id_sequence);
    STATE(argument_ids)[key] = argument_id;
    return argument_id;
}

static inline arglist_t get_arguments(SEXP op, SEXP rho) {
    arglist_t arguments;

    for (SEXP formals = FORMALS(op); formals != R_NilValue; formals = CDR(formals)) {
        // Retrieve the argument name.
        SEXP argument_expression = TAG(formals);
        SEXP promise_expression = get_promise(argument_expression, rho);

        if (TYPEOF(promise_expression) == DOTSXP) {
            int i = 0;
            for(SEXP dots = promise_expression; dots != R_NilValue; dots = CDR(dots)) {
                SEXP ddd_argument_expression = TAG(dots);
                SEXP ddd_promise_expression = CAR(dots);
                if (ddd_argument_expression == R_NilValue) {
                    arguments.push_back({
                                                get_argument_id(get_function_id(op), to_string(i++)),
                                                get_promise_id(ddd_promise_expression)
                                        }); // ... argument without a name
                }
                else {
                    string ddd_arg_name = get_name(ddd_argument_expression);
                    arguments.push_back({
                                                ddd_arg_name,
                                                get_argument_id(get_function_id(op), ddd_arg_name),
                                                get_promise_id(ddd_promise_expression)
                                        }, true); // this flag says we're inserting a ... argument
                }
            }
        }
        else {
            // Retrieve the promise for the argument.
            // The call SEXP only contains AST to find the actual argument value, we need to search the environment.
            string arg_name = get_name(argument_expression);
            prom_id_t prom_id = get_promise_id(promise_expression);
            if (prom_id != RID_INVALID)
                arguments.push_back({
                                            arg_name,
                                            get_argument_id(get_function_id(op), arg_name),
                                            prom_id
                                    });
        }

    }

    return arguments;
}

// All the interpreter hooks go here
// DECL_HOOK macro generates an initializer for each function
// which is then used in the REGISTER_HOOKS macro to properly init rdt_handler.
struct trace_promises {
    // ??? can we get metadata about the program we're analysing in here?
    // TODO: also pass environment
    DECL_HOOK(begin)(const SEXP prom) {
        tracer_state().start_pass(prom);
    }

    DECL_HOOK(end)() {
        tracer_state().finish_pass();

        if (output) {
            fclose(output);
            output = NULL;
        }
    }

    // Triggered when entering function evaluation.
    DECL_HOOK(function_entry)(const SEXP call, const SEXP op, const SEXP rho) {
        const char *type = is_byte_compiled(call) ? "=> bcod" : "=> func";
        int call_type = is_byte_compiled(call) ? 1 : 0;
        const char *name = get_name(call);
        const char *ns = get_ns_name(op);
        fn_addr_t fn_id = get_function_id(op);
        env_addr_t call_ptr = get_sexp_address(rho);
#ifdef RDT_CALL_ID
        call_id_t call_id = make_funcall_id(op);
#else
        call_id_t call_id = make_funcall_id(rho);
#endif
        char *loc = get_location(op);
        char *fqfn = NULL;

        if (ns) {
            asprintf(&fqfn, "%s::%s", ns, CHKSTR(name));
        } else {
            fqfn = name != NULL ? strdup(name) : NULL;
        }

        // Push function ID on function stack
        STATE(fun_stack).push(call_id);
#ifdef RDT_CALL_ID
        STATE(curr_env_stack).push(call_ptr);
#endif

        arglist_t arguments = get_arguments(op, rho);

        // if (SQL call)
        //    function_definition <- deparse1line(function)
        // otherwise function_definition <- NULL;
        const char* fn_definition = NULL;
        if (tracer_conf.output_format != RDT_OUTPUT_TRACE)
            fn_definition = get_expression(op);
        //deparse1line(op, FALSE);
        //R_inspect(deparsed_function);
        // FIXME ESCAPE fn_definition (prepared statements?)
        //Rprintf(fn_definition);

        // TODO link with mk_sql
        // TODO rename to reflect non-printing nature
        // TODO meh, ugly
        if (tracer_conf.output_format == RDT_OUTPUT_COMPILED_SQLITE && tracer_conf.output_type == RDT_SQLITE) {
            run_prep_sql_function(fn_id, arguments, loc, fn_definition);
            run_prep_sql_function_call(call_id, call_ptr, fqfn, loc, call_type, fn_id);
            run_prep_sql_promise_assoc(arguments, call_id);
        } else {
            rdt_print(RDT_OUTPUT_TRACE, {print_function(type, loc, fqfn, fn_id, call_id, arguments)});

            rdt_print(RDT_OUTPUT_SQL, {mk_sql_function(fn_id, arguments, loc, fn_definition),
                                       mk_sql_function_call(call_id, call_ptr, fqfn, loc, call_type, fn_id),
                                       mk_sql_promise_assoc(arguments, call_id)});
        }

        if (tracer_conf.pretty_print)
            STATE(indent) += tracer_conf.indent_width;

        auto & fresh_promises = STATE(fresh_promises);
        // Associate promises with call ID
        for (auto arg_ref : arguments.all()) {
            const arg_t & argument = arg_ref.get();
            auto & promise = get<2>(argument);
            auto it = fresh_promises.find(promise);

            if (it != fresh_promises.end()) {
                STATE(promise_origin)[promise] = call_id;
                fresh_promises.erase(it);
            }
        }

        if (loc)
            free(loc);
        if (fqfn)
            free(fqfn);
    }

    DECL_HOOK(function_exit)(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
        if (tracer_conf.pretty_print)
            STATE(indent) -= tracer_conf.indent_width;

        const char *type = is_byte_compiled(call) ? "<= bcod" : "<= func";
        const char *name = get_name(call);
        const char *ns = get_ns_name(op);
        fn_addr_t fn_id = get_function_id(op);
        call_id_t call_id = STATE(fun_stack).top();
        char *loc = get_location(op);
        char *fqfn = NULL;

        if (ns) {
            asprintf(&fqfn, "%s::%s", ns, CHKSTR(name));
        } else {
            fqfn = name != NULL ? strdup(name) : NULL;
        }

        arglist_t arguments = get_arguments(op, rho);

        rdt_print(RDT_OUTPUT_TRACE, {print_function(type, loc, fqfn, fn_id, call_id, arguments)});

        // Pop current function ID
        STATE(fun_stack).pop();
#ifdef RDT_CALL_ID
        STATE(curr_env_stack).pop();
#endif

        if (loc)
            free(loc);
        if (fqfn)
            free(fqfn);
    }

    // TODO retrieve arguments
    DECL_HOOK(builtin_entry)(const SEXP call, const SEXP op, const SEXP rho) {
        const char *name = get_name(call);
        fn_addr_t fn_id = get_function_id(op);

        env_addr_t call_ptr = get_sexp_address(rho);
#ifdef RDT_CALL_ID
        call_id_t call_id = make_funcall_id(op);
#else
        // Builtins have no environment of their own
        // we take the parent env rho and add 1 to it to create a new pseudo-address
        // it will be unique because real pointers are aligned (no odd addresses)
        call_id_t call_id = make_funcall_id(rho) | 1;
#endif

        // TODO merge rdt_print_calls
        rdt_print(RDT_OUTPUT_TRACE, {print_builtin("=> b-in", NULL, name, fn_id, call_id)});

        arglist_t arguments;

        if (tracer_conf.output_format == RDT_OUTPUT_COMPILED_SQLITE && tracer_conf.output_type == RDT_SQLITE) {
            run_prep_sql_function(fn_id, arguments, NULL, NULL);
            run_prep_sql_function_call(call_id, call_ptr, name, NULL, 1, fn_id);
            //run_prep_sql_promise_assoc(arguments, call_id);
        } else {
            rdt_print(RDT_OUTPUT_SQL, {mk_sql_function(fn_id, arguments, NULL, NULL),
                                       mk_sql_function_call(call_id, call_ptr, name, NULL, 1, fn_id)});
        }
        // mk_sql_promises(promises, call_id, argument_ids)

        STATE(fun_stack).push(call_id);
#ifdef RDT_CALL_ID
        STATE(curr_env_stack).push(call_ptr | 1);
#endif

        if (tracer_conf.pretty_print)
            STATE(indent) += tracer_conf.indent_width;
    }

    DECL_HOOK(builtin_exit)(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
        const char *name = get_name(call);
        fn_addr_t id = get_function_id(op);
        call_id_t call_id = STATE(fun_stack).top();

        STATE(fun_stack).pop();
#ifdef RDT_CALL_ID
        STATE(curr_env_stack).pop();
#endif
        if (tracer_conf.pretty_print)
            STATE(indent) -= tracer_conf.indent_width;

        rdt_print(RDT_OUTPUT_TRACE, {print_builtin("<= b-in", NULL, name, id, call_id)});
    }

    DECL_HOOK(promise_created)(const SEXP prom) {
        prom_id_t prom_id = make_promise_id(prom);
        STATE(fresh_promises).insert(prom_id);

        //Rprintf("PROMISE CREATED at %p\n", get_sexp_address(prom));
        //TODO implement promise allocation pretty print
        //rdt_print(RDT_OUTPUT_TRACE, {print_promise_alloc(prom_id)});

        if (tracer_conf.output_format == RDT_OUTPUT_COMPILED_SQLITE && tracer_conf.output_type == RDT_SQLITE) {
            run_prep_sql_promise(prom_id);
        } else {
            rdt_print(RDT_OUTPUT_SQL, {mk_sql_promise(prom_id)});
        }
    }

    // Promise is being used inside a function body for the first time.
    DECL_HOOK(force_promise_entry)(const SEXP symbol, const SEXP rho) {
        const char *name = get_name(symbol);

        SEXP promise_expression = get_promise(symbol, rho);
        prom_id_t id = get_promise_id(promise_expression);
        call_id_t in_call_id = STATE(fun_stack).top();
        call_id_t from_call_id = STATE(promise_origin)[id];

        // in_call_id = current call
        if (tracer_conf.output_format == RDT_OUTPUT_COMPILED_SQLITE && tracer_conf.output_type == RDT_SQLITE) {
            run_prep_sql_promise_evaluation(RDT_FORCE_PROMISE, id, from_call_id);
        } else {
            rdt_print(RDT_OUTPUT_TRACE, {print_promise("=> prom", NULL, name, id, in_call_id, from_call_id)});
            rdt_print(RDT_OUTPUT_SQL, {mk_sql_promise_evaluation(RDT_FORCE_PROMISE, id, from_call_id)});
        }
    }

    DECL_HOOK(force_promise_exit)(const SEXP symbol, const SEXP rho, const SEXP val) {
        const char *name = get_name(symbol);

        SEXP promise_expression = get_promise(symbol, rho);
        prom_id_t id = get_promise_id(promise_expression);
        call_id_t in_call_id = STATE(fun_stack).top();
        call_id_t from_call_id = STATE(promise_origin)[id];

        rdt_print(RDT_OUTPUT_TRACE, {print_promise("<= prom", NULL, name, id, in_call_id, from_call_id)});
    }

    DECL_HOOK(promise_lookup)(const SEXP symbol, const SEXP rho, const SEXP val) {
        const char *name = get_name(symbol);

        SEXP promise_expression = get_promise(symbol, rho);
        prom_id_t id = get_promise_id(promise_expression);
        call_id_t in_call_id = STATE(fun_stack).top();
        call_id_t from_call_id = STATE(promise_origin)[id];

        // TODO
        if (tracer_conf.output_format == RDT_OUTPUT_COMPILED_SQLITE && tracer_conf.output_type == RDT_SQLITE) {
            run_prep_sql_promise_evaluation(RDT_LOOKUP_PROMISE, id, from_call_id);
        } else {
            rdt_print(RDT_OUTPUT_TRACE, {print_promise("<> lkup", NULL, name, id, in_call_id, from_call_id)});
            rdt_print(RDT_OUTPUT_SQL, {mk_sql_promise_evaluation(RDT_LOOKUP_PROMISE, id, from_call_id)});
        }
    }

    DECL_HOOK(error)(const SEXP call, const char* message) {
        char *call_str = NULL;
        char *loc = get_location(call);

        asprintf(&call_str, "\"%s\"", get_call(call));

        if (loc) free(loc);
        if (call_str) free(call_str);
    }

    DECL_HOOK(vector_alloc)(int sexptype, long length, long bytes, const char* srcref) {
    }

//    DECL_HOOK(eval_entry)(SEXP e, SEXP rho) {
//        switch(TYPEOF(e)) {
//            case LANGSXP:
//                fprintf(output, "%s\n");
//                PrintValue
//            break;
//        }
//    }
//
//    DECL_HOOK(eval_exit)(SEXP e, SEXP rho, SEXP retval) {
//        printf("");
//    }

    DECL_HOOK(gc_promise_unmarked)(const SEXP promise) {
        prom_addr_t addr = get_sexp_address(promise);
        prom_id_t id = get_promise_id(promise);
        auto & promise_origin = STATE(promise_origin);

        auto iter = promise_origin.find(id);
        if (iter != promise_origin.end()) {
            // If this is one of our traced promises,
            // delete it from origin map because it is ready to be GCed
            promise_origin.erase(iter);
            //Rprintf("Promise %#x deleted.\n", id);
        }

        STATE(promise_ids).erase(addr);
    }

    DECL_HOOK(jump_ctxt)(const SEXP rho, const SEXP val) {
        tracer_state().adjust_fun_stack(rho);
    }
};

// TODO: move to trace_promises struct and add DECL_HOOK macro, if we need these
static void trace_promises_gc_entry(R_size_t size_needed) {
}

static void trace_promises_gc_exit(int gc_count, double vcells, double ncells) {
}

static void trace_promises_S3_generic_entry(const char *generic, const SEXP object) {
}

static void trace_promises_S3_generic_exit(const char *generic, const SEXP object, const SEXP retval) {
}

static void trace_promises_S3_dispatch_entry(const char *generic, const char *clazz, const SEXP method, const SEXP object) {
}

static void trace_promises_S3_dispatch_exit(const char *generic, const char *clazz, const SEXP method, const SEXP object, const SEXP retval) {
}

tracer_conf_t get_config_from_R_options(SEXP options) {
    tracer_conf_t conf;

    const char *filename_option = get_string(get_named_list_element(options, "path"));
    if (filename_option != NULL)
        conf.filename = filename_option;

    const char *output_format_option = get_string(get_named_list_element(options, "format"));
    if (output_format_option != NULL) {
        if (!strcmp(output_format_option, "trace"))
            conf.output_format = RDT_OUTPUT_TRACE;
        else if (!strcmp(output_format_option, "SQL") || !strcmp(output_format_option, "sql"))
            conf.output_format = RDT_OUTPUT_SQL;
	else if (!strcmp(output_format_option, "PSQL") || !strcmp(output_format_option, "psql"))
            conf.output_format = RDT_OUTPUT_COMPILED_SQLITE;
        else if (!strcmp(output_format_option, "both"))
            conf.output_format = RDT_OUTPUT_BOTH;
        else
            error("Unknown format type: \"%s\"\n", output_format_option);
    }

    //Rprintf("output_format_option=%s->%i\n", output_format_option,output_format);

    const char *output_type_option = get_string(get_named_list_element(options, "output"));
    if (output_type_option != NULL) {
        if (!strcmp(output_type_option, "R") || !strcmp(output_type_option, "r"))
            conf.output_type = RDT_R_PRINT;
        else if (!strcmp(output_type_option, "file"))
            conf.output_type = RDT_FILE;
        else if (!strcmp(output_type_option, "DB") || !strcmp(output_type_option, "db"))
            conf.output_type = RDT_SQLITE;
        else if (!strcmp(output_type_option, "R+DB") || !strcmp(output_type_option, "r+db") ||
                 !strcmp(output_type_option, "DB+R") || !strcmp(output_type_option, "db+r"))
            conf.output_type = RDT_R_PRINT_AND_SQLITE;
        else
            error("Unknown format type: \"%s\"\n", output_type_option);
    }

    //Rprintf("output_type_option=%s->%i\n", output_type_option,output_type);

    SEXP pretty_print_option = get_named_list_element(options, "pretty.print");
    if (pretty_print_option != NULL && pretty_print_option != R_NilValue)
        conf.pretty_print = LOGICAL(pretty_print_option)[0] == TRUE;
    //Rprintf("pretty_print_option=%p->%i\n", (pretty_print_option), pretty_print);

    SEXP indent_width_option = get_named_list_element(options, "indent.width");
    if (indent_width_option != NULL && indent_width_option != R_NilValue)
        if (TYPEOF(indent_width_option) == REALSXP)
            conf.indent_width = (int) *REAL(indent_width_option);
    //Rprintf("indent_width_option=%p->%i\n", indent_width_option, indent_width);

    SEXP overwrite_option = get_named_list_element(options, "overwrite");
    if (overwrite_option != NULL && overwrite_option != R_NilValue)
        conf.overwrite = LOGICAL(overwrite_option)[0] == TRUE;
    //Rprintf("overwrite_option=%p->%i\n", (overwrite_option), overwrite);

#ifdef RDT_CALL_ID
    SEXP synthetic_call_id_option = get_named_list_element(options, "synthetic.call.id");
    if (synthetic_call_id_option != NULL && synthetic_call_id_option == R_NilValue)
        conf.call_id_use_ptr_fmt = LOGICAL(synthetic_call_id_option)[0] == FALSE;
    //Rprintf("call_id_use_ptr_fmt=%p->%i\n", (synthetic_call_id_option), call_id_use_ptr_fmt);
#endif

    return conf;
}

static bool file_exists(const string & fname) {
    ifstream f(fname);
    return f.good();
}

rdt_handler *setup_promise_tracing(SEXP options) {
    tracer_conf_t new_conf = get_config_from_R_options(options);
    tracer_conf.update(new_conf);

    if (tracer_conf.output_type != RDT_SQLITE && tracer_conf.output_type != RDT_R_PRINT_AND_SQLITE) {
        output = fopen(tracer_conf.filename->c_str(), tracer_conf.overwrite ? "w" : "a");
        if (!output) {
            error("Unable to open %s: %s\n", tracer_conf.filename, strerror(errno));
            return NULL;
        }
    }

//    THIS IS DONE IN tracer_state().start_pass() (called from trace_promises_begin()) if the overwrite flag is set
//    call_id_counter = 0;
//    already_inserted_functions.clear();
//
    if(tracer_conf.overwrite && (tracer_conf.output_type == RDT_SQLITE || tracer_conf.output_type == RDT_R_PRINT_AND_SQLITE)) {
        if (file_exists(tracer_conf.filename)) {
            remove(tracer_conf.filename->c_str());
        }
    }

    if (tracer_conf.output_type == RDT_SQLITE || tracer_conf.output_type == RDT_R_PRINT_AND_SQLITE)
        rdt_init_sqlite(tracer_conf.filename);


    if (tracer_conf.output_type != RDT_OUTPUT_TRACE) {
        rdt_configure_sqlite();
        rdt_begin_transaction();
    }

    rdt_handler *h = (rdt_handler *) malloc(sizeof(rdt_handler));
    //memcpy(h, &trace_promises_rdt_handler, sizeof(rdt_handler));
    //*h = trace_promises_rdt_handler; // This actually does the same thing as memcpy
    *h = REGISTER_HOOKS(trace_promises,
                        tr::begin,
                        tr::end,
                        tr::function_entry,
                        tr::function_exit,
                        tr::builtin_entry,
                        tr::builtin_exit,
                        tr::force_promise_entry,
                        tr::force_promise_exit,
                        tr::promise_lookup,
                        tr::error,
                        tr::vector_alloc,
                        tr::gc_promise_unmarked,
                        tr::jump_ctxt,
                        tr::promise_created);

    SEXP disabled_probes = get_named_list_element(options, "disabled.probes");
    if (disabled_probes != R_NilValue && TYPEOF(disabled_probes) == STRSXP) {
        for (int i=0; i<LENGTH(disabled_probes); i++) {
            const char *probe = CHAR(STRING_ELT(disabled_probes, i));

            if (!strcmp("function", probe)) {
                h->probe_function_entry = NULL;
                h->probe_function_exit = NULL;
            } else if (!strcmp("builtin", probe)) {
                h->probe_builtin_entry = NULL;
                h->probe_builtin_exit = NULL;
            } else if (!strcmp("promise", probe)) {
                h->probe_promise_lookup = NULL;
                h->probe_force_promise_entry = NULL;
                h->probe_force_promise_exit = NULL;
            } else if (!strcmp("vector", probe)) {
                h->probe_vector_alloc = NULL;
            } else if (!strcmp("gc", probe)) {
                h->probe_gc_entry = NULL;
                h->probe_gc_exit = NULL;
            } else if (!strcmp("S3", probe)) {
                h->probe_S3_dispatch_entry = NULL;
                h->probe_S3_dispatch_exit = NULL;
                h->probe_S3_generic_entry = NULL;
                h->probe_S3_generic_exit = NULL;
            } else {
                warning("Unknown probe `%s`\n", probe);
            }
        }
    }

    //rdt_close_sqlite();
    return h;
}

void cleanup_promise_tracing(/*rdt_handler *h,*/ SEXP options) {
    if (tracer_conf.output_type != RDT_OUTPUT_TRACE)
        rdt_commit_transaction();

    if (tracer_conf.output_type == RDT_SQLITE || tracer_conf.output_type == RDT_R_PRINT_AND_SQLITE)
        rdt_close_sqlite();
}
