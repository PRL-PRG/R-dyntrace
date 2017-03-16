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
#endif

// SQL specific
#include <unordered_set>

//#include <Defn.h>

//#define RDT_PROMSES_SQL_SUPPORT
//#ifdef RDT_PROMSES_SQL_SUPPORT
//#include <sqlite3.h>
//sqlite3 *db;
//#endif

extern "C" {
#include "../../../main/inspect.h"
}

#include "rdt.h"

using namespace std;

// Use generated call IDs instead of function env addresses
#define RDT_CALL_ID

#define RDT_LOOKUP_PROMISE 0x0
#define RDT_FORCE_PROMISE 0xF

typedef uintptr_t rid_t;
typedef int sid_t;

#define RID_INVALID -1

static FILE *output = NULL;


enum OutputFormat: char {RDT_OUTPUT_TRACE, RDT_OUTPUT_SQL, RDT_OUTPUT_BOTH};
enum Output: char {RDT_R_PRINT, RDT_FILE, RDT_SQLITE, RDT_R_PRINT_AND_SQLITE};

struct tracer_conf_t {
    const char * filename;
    int output_type;
    int output_format;
    bool pretty_print;
    bool overwrite;
    int indent_width;
    //static bool synthetic_call_id = true;
    bool call_id_use_ptr_fmt;
    bool first_update;

    tracer_conf_t() {
        // Config defaults
        filename = NULL;
        output_type = RDT_R_PRINT_AND_SQLITE;
        output_format = RDT_OUTPUT_BOTH;
        pretty_print = true;
        overwrite = false;
        indent_width = 4;
#ifdef RDT_CALL_ID
        call_id_use_ptr_fmt = false;
#else
        call_id_use_ptr_fmt = true;
#endif
        first_update = true;
    }

    // Update configuration in a smart way
    // (e.g. ignore changes of output type/format if overwrite == false)
    void update(const tracer_conf_t & conf) {
        if (first_update || conf.overwrite) {
            *this = conf; // overwrites all members
            first_update = false;
        }
        else {
            //overwrite = conf.overwrite;
            // This is the same as the above line but we know conf.overwrite cannot be true (because we're in this branch)
            overwrite = false;
        }
    }
};

static tracer_conf_t tracer_conf; // init default configuration

static inline rid_t get_sexp_address(SEXP e) {
    return (rid_t)e;
}

static inline void prepend_prefix(stringstream *stream);

static inline string print_unwind(const char *type, rid_t call_id) {
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

typedef pair<rid_t, string> arg_key_t;

struct tracer_state_t {
    int indent;
    // Function call stack (may be useful)
    // Whenever R makes a function call, we generate a function ID and store that ID on top of the stack
    // so that we know where we are (e.g. when printing function ID at function_exit hook)
    stack<rid_t, vector<rid_t>> fun_stack; // Should be reset on each tracer pass
#ifdef RDT_CALL_ID
#define CALL_ID_FMT "%d"
    stack<rid_t, vector<rid_t>> curr_env_stack; // Should be reset on each tracer pass
#else
#define CALL_ID_FMT "%#x"
#endif

    // Map from promise IDs to call IDs
    unordered_map<rid_t, rid_t> promise_origin; // Should be reset on each tracer pass


    int call_id_counter; // IDs assigned should be globally unique but we can reset it after each pass if overwrite is true)
    unordered_set<rid_t> already_inserted_functions; // Should be kept across Rdt calls (unless overwrite is true)
    unordered_set<rid_t> already_inserted_promises; // Should be kept across Rdt calls (unless overwrite is true)
    sid_t argument_id_sequence; // Should be globally unique (can reset between tracer calls if overwrite is true)
    map<arg_key_t, sid_t> argument_ids; // Should be kept across Rdt calls (unless overwrite is true)

    void start_pass() {
        if (tracer_conf.overwrite) {
            reset();
        }

        indent = 0;

        // We have to make sure the stack is not empty
        // when referring to the promise created by call to Rdt.
        // This is just a dummy call and environment.
        fun_stack.push(0);
#ifdef RDT_CALL_ID
        curr_env_stack.push(0);
#endif
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
        rid_t call_id, call_addr;

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
        call_id_counter = 0;
        argument_id_sequence = 0;
    }

    void reset() {
        call_id_counter = 0;
        argument_id_sequence = 0;
        already_inserted_functions.clear();
        already_inserted_promises.clear();
        argument_ids.clear();
    }
};

static inline tracer_state_t& tracer_state() {
    return tracer_state_t::get_instance();
}

// Helper macro for accessing state properties
#define STATE(property) tracer_state().property

//typedef vector<rid_t> prom_vec_t;
typedef tuple<string, sid_t, rid_t> arg_t;
typedef tuple<sid_t, rid_t> anon_arg_t;

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


static inline void rdt_init_sqlite(const char *filename) {
#ifdef RDT_SQLITE_SUPPORT
    int outcome;
    char *error_msg = NULL;
    outcome = sqlite3_open(filename, &sqlite_database);

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
#else
    fprintf(stderr, "-- SQLite support is missing...?\n");
#endif
}

static inline void rdt_close_sqlite() {
#ifdef RDT_SQLITE_SUPPORT
    sqlite3_close(sqlite_database);
#endif
}

static inline string print_builtin(const char *type, const char *loc, const char *name, rid_t id) {
    stringstream stream;
    prepend_prefix(&stream);

    if (tracer_conf.pretty_print && (tracer_conf.output_format == RDT_OUTPUT_TRACE))
        stream << string(STATE(indent), ' ');

    stream << type << " "
           << "loc(" << CHKSTR(loc) << ") "
           << "fun(" << CHKSTR(name) << "=" << hex << id << ")\n";

    return stream.str();
}

static inline string print_promise(const char *type, const char *loc, const char *name, rid_t id, rid_t in_call_id, rid_t from_call_id) {
    stringstream stream;
    prepend_prefix(&stream);

    if (tracer_conf.pretty_print && (tracer_conf.output_format == RDT_OUTPUT_TRACE))
        stream << string(STATE(indent), ' ');

    auto num_fmt = tracer_conf.call_id_use_ptr_fmt ? hex : dec;

    stream << type << " "
           << "loc(" << CHKSTR(loc) << ") "
           << "prom(" << CHKSTR(name) << "=0x" << hex << id << ") ";
    stream << "in(" << num_fmt << in_call_id << ") ";
    stream << "from(" << num_fmt << from_call_id << ")\n";

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

static inline string mk_sql_function(rid_t function_id, arglist_t const& arguments, const char* location, const char* definition) {
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

static inline string mk_sql_function_call(rid_t call_id, rid_t call_ptr, const char *name, const char* location, int call_type, rid_t function_id) {
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

static inline string mk_sql_promises(arglist_t const& arguments, rid_t call_id) {
    std::stringstream promise_stream;
    std::stringstream promise_association_stream;

    int inserted_promises = 0;

    if (arguments.size() > 0) {
        promise_stream << (tracer_conf.pretty_print ? "insert into promises  select"
                                                    : "insert into promises select");
        promise_association_stream << (
                tracer_conf.pretty_print ? "insert into promise_associations select"
                                         : "insert into promise_associations select");
        int index = 0;
        for (auto arg_ref : arguments.all()) {
            const arg_t & argument = arg_ref.get();
            sid_t arg_id = get<1>(argument);
            rid_t promise = get<2>(argument);

            bool promise_already_inserted = STATE(already_inserted_promises).count(promise);
            if (!promise_already_inserted) {
                STATE(already_inserted_promises).insert(promise);

                if (inserted_promises)
                    promise_stream << (tracer_conf.pretty_print ? "\n            " : " ")
                                   << "union all select";

                promise_stream << " 0x" << hex << promise;

                inserted_promises++;
            }

            if (index)
                promise_association_stream << (tracer_conf.pretty_print ? "\n            " : " ")
                                           << "union all select";

            promise_association_stream << " "
                           << "0x" << hex << promise << ","
                           << dec << call_id << ","
                           << dec << arg_id;

            index++;
        }
        promise_association_stream << ";\n";

        if (inserted_promises) {
            promise_stream << ";\n";
            promise_stream << promise_association_stream.str();
        }
    }

    return inserted_promises ? promise_stream.str() : promise_association_stream.str();
}

static inline string mk_sql_promise_evaluation(int event_type, rid_t promise_id, rid_t call_id) {
    std::stringstream stream;
    stream << "insert into promise_evaluations values ("
           << "$next_id,"
           << "0x" << hex << event_type << ",";
    stream << "0x" << hex << promise_id << ",";
    stream << dec << call_id
           << ");\n";
    return stream.str();
}


static inline string print_function(const char *type, const char *loc, const char *name, rid_t function_id, rid_t call_id, arglist_t const& arguments) {
    stringstream stream;
    prepend_prefix(&stream);

    if (tracer_conf.pretty_print && (tracer_conf.output_format == RDT_OUTPUT_TRACE))
        stream << string(STATE(indent), ' ');

    stream << type << " "
           << "loc(" << CHKSTR(loc) << ") "
           << "call(" << (tracer_conf.call_id_use_ptr_fmt ? hex : dec) << call_id << ") ";
    stream << "fun(" << CHKSTR(name) << "=" << hex << function_id << ") ";

    // print argument names and the promises bound to them
    stream << "arguments(";
    int i = 0;
    for (auto arg_ref : arguments.all()) {
        const arg_t & argument = arg_ref.get();
        rid_t promise = get<2>(argument);
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


// ??? can we get metadata about the program we're analysing in here?
static void trace_promises_begin() {
    tracer_state().start_pass();
}

static void trace_promises_end() {
    tracer_state().finish_pass();

    if (output)
        fclose(output);
}

static inline int count_elements(SEXP list) {
    int counter = 0;
    SEXP tmp = list;
    for (; tmp != R_NilValue; counter++)
        tmp = CDR(tmp);
    return counter;
}

static inline rid_t get_promise_id(SEXP promise) {
    if (promise == R_NilValue)
        return RID_INVALID;
    if (TYPEOF(promise) != PROMSXP)
        return RID_INVALID;

    // A new promise is always created for each argument.
    // Even if the argument is already a promise passed from the caller, it gets re-wrapped.
    return get_sexp_address(promise);
}

static inline rid_t get_function_id(SEXP func) {
    assert(TYPEOF(func) == CLOSXP);
    return get_sexp_address(func);
}

#ifdef RDT_CALL_ID
static inline rid_t make_funcall_id(SEXP function) {
    if (function == R_NilValue)
        return RID_INVALID;

    return ++STATE(call_id_counter);
}
#else
static inline rid_t make_funcall_id(SEXP fn_env) {
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


static inline sid_t get_argument_id(rid_t function_id, const string & argument) {
    arg_key_t key = make_pair(function_id, argument);
    auto iterator = STATE(argument_ids).find(key);

    if (iterator != STATE(argument_ids).end()) {
        return iterator->second;
    }

    sid_t argument_id = ++STATE(argument_id_sequence);
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
            rid_t prom_id = get_promise_id(promise_expression);
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

// Triggered when entering function evaluation.
static void trace_promises_function_entry(const SEXP call, const SEXP op, const SEXP rho) {
    const char *type = is_byte_compiled(call) ? "=> bcod" : "=> func";
    int call_type = is_byte_compiled(call) ? 1 : 0;
    const char *name = get_name(call);
    const char *ns = get_ns_name(op);
    rid_t fn_id = get_function_id(op);
    rid_t call_ptr = get_sexp_address(rho);
#ifdef RDT_CALL_ID
    rid_t call_id = make_funcall_id(op);
#else
    rid_t call_id = make_funcall_id(rho);
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
    STATE(curr_env_stack).push(get_sexp_address(rho));
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
    rdt_print(RDT_OUTPUT_TRACE, {print_function(type, loc, fqfn, fn_id, call_id, arguments)});

    rdt_print(RDT_OUTPUT_SQL, {mk_sql_function(fn_id, arguments, loc, fn_definition),
               mk_sql_function_call(call_id, call_ptr, name, loc, call_type, fn_id),
               mk_sql_promises(arguments, call_id)});

    if (tracer_conf.pretty_print)
        STATE(indent) += tracer_conf.indent_width;

    // Associate promises with call ID
    for (auto arg_ref : arguments.all()) {
        const arg_t & argument = arg_ref.get();
        auto & promise = get<2>(argument);
        STATE(promise_origin)[promise] = call_id;
    }

    if (loc)
        free(loc);
    if (fqfn)
        free(fqfn);
}

static void trace_promises_function_exit(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
    if (tracer_conf.pretty_print)
        STATE(indent) -= tracer_conf.indent_width;

    const char *type = is_byte_compiled(call) ? "<= bcod" : "<= func";
    const char *name = get_name(call);
    const char *ns = get_ns_name(op);
    rid_t fn_id = get_function_id(op);
    rid_t call_id = STATE(fun_stack).top();
    char *loc = get_location(op);
    char *fqfn = NULL;

    if (ns) {
        asprintf(&fqfn, "%s::%s", ns, CHKSTR(name));
    } else {
        fqfn = name != NULL ? strdup(name) : NULL;
    }

    arglist_t arguments = get_arguments(op, rho);

    rdt_print(RDT_OUTPUT_TRACE, {print_function(type, loc, name, fn_id, call_id, arguments)});

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
static void trace_promises_builtin_entry(const SEXP call, const SEXP op, const SEXP rho) {
    const char *name = get_name(call);
    rid_t fn_id = get_function_id(op);

    rid_t call_ptr = get_sexp_address(rho);
#ifdef RDT_CALL_ID
    rid_t call_id = make_funcall_id(op);
#else
    rid_t call_id = make_funcall_id(rho);
#endif

    // TODO merge rdt_print_calls
    rdt_print(RDT_OUTPUT_TRACE, {print_builtin("=> b-in", NULL, name, fn_id)});

    arglist_t arguments;

    rdt_print(RDT_OUTPUT_SQL, {mk_sql_function(fn_id, arguments, NULL, NULL),
               mk_sql_function_call(call_id, call_ptr, name, NULL, 1, fn_id)});
            // mk_sql_promises(promises, call_id, argument_ids)

    //R_inspect(call);
}

static void trace_promises_builtin_exit(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
    const char *name = get_name(call);
    rid_t id = get_function_id(op);

    rdt_print(RDT_OUTPUT_TRACE, {print_builtin("<= b-in", NULL, name, id)});
}

// Promise is being used inside a function body for the first time.
static void trace_promises_force_promise_entry(const SEXP symbol, const SEXP rho) {
    const char *name = get_name(symbol);

    SEXP promise_expression = get_promise(symbol, rho);
    rid_t id = get_promise_id(promise_expression);
    rid_t in_call_id = STATE(fun_stack).top();
    rid_t from_call_id = STATE(promise_origin)[id];

    // in_call_id = current call
    rdt_print(RDT_OUTPUT_TRACE, {print_promise("=> prom", NULL, name, id, in_call_id, from_call_id)});
    rdt_print(RDT_OUTPUT_SQL, {mk_sql_promise_evaluation(RDT_FORCE_PROMISE, id, from_call_id)});

}

static void trace_promises_force_promise_exit(const SEXP symbol, const SEXP rho, const SEXP val) {
    const char *name = get_name(symbol);

    SEXP promise_expression = get_promise(symbol, rho);
    rid_t id = get_promise_id(promise_expression);
    rid_t in_call_id = STATE(fun_stack).top();
    rid_t from_call_id = STATE(promise_origin)[id];

    rdt_print(RDT_OUTPUT_TRACE, {print_promise("<= prom", NULL, name, id, in_call_id, from_call_id)});
}

static void trace_promises_promise_lookup(const SEXP symbol, const SEXP rho, const SEXP val) {
    const char *name = get_name(symbol);

    SEXP promise_expression = get_promise(symbol, rho);
    rid_t id = get_promise_id(promise_expression);
    rid_t in_call_id = STATE(fun_stack).top();
    rid_t from_call_id = STATE(promise_origin)[id];

    // TODO
    rdt_print(RDT_OUTPUT_TRACE, {print_promise("<> lkup", NULL, name, id, in_call_id, from_call_id)});
    rdt_print(RDT_OUTPUT_SQL, {mk_sql_promise_evaluation(RDT_LOOKUP_PROMISE, id, from_call_id)});

}

static void trace_promises_error(const SEXP call, const char* message) {
    char *call_str = NULL;
    char *loc = get_location(call);

    asprintf(&call_str, "\"%s\"", get_call(call));

    if (loc) free(loc);
    if (call_str) free(call_str);
}

static void trace_promises_vector_alloc(int sexptype, long length, long bytes, const char* srcref) {
}

// static void trace_eval_entry(SEXP e, SEXP rho) {
//     switch(TYPEOF(e)) {
//         case LANGSXP:
//             fprintf(output, "%s\n");
//             PrintValue
//         break;
//     }
// }

// static void trace_eval_exit(SEXP e, SEXP rho, SEXP retval) {
//     printf("");
// }

static void trace_promises_gc_promise_unmarked(const SEXP promise) {
    rid_t id = (rid_t)promise;
    auto & promise_origin = STATE(promise_origin);

    auto iter = promise_origin.find(id);
    if (iter != promise_origin.end()) {
        // If this is one of our traced promises,
        // delete it from origin map because it is ready to be GCed
        promise_origin.erase(iter);
        //Rprintf("Promise %#x deleted.\n", id);
    }
}

static void trace_promises_jump_ctxt(const SEXP rho, const SEXP val) {
    tracer_state().adjust_fun_stack(rho);
}

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

// TODO properly turn off probes we don't use
static const rdt_handler trace_promises_rdt_handler = {
        &trace_promises_begin,
        &trace_promises_end,
        &trace_promises_function_entry,
        &trace_promises_function_exit,
        &trace_promises_builtin_entry,
        &trace_promises_builtin_exit,
        &trace_promises_force_promise_entry,
        &trace_promises_force_promise_exit,
        &trace_promises_promise_lookup,
        &trace_promises_error,
        &trace_promises_vector_alloc,
        NULL, // &trace_eval_entry,
        NULL, // &trace_eval_exit,
        &trace_promises_gc_entry,
        &trace_promises_gc_exit,
        &trace_promises_gc_promise_unmarked,
        &trace_promises_jump_ctxt,
        &trace_promises_S3_generic_entry,
        &trace_promises_S3_generic_exit,
        &trace_promises_S3_dispatch_entry,
        &trace_promises_S3_dispatch_exit
};

tracer_conf_t get_config_from_R_options(SEXP options) {
    tracer_conf_t conf;

    conf.filename = get_string(get_named_list_element(options, "path"));
    if (conf.filename == NULL)
        conf.filename = "trace.db";

    const char *output_format_option = get_string(get_named_list_element(options, "format"));
    if (output_format_option == NULL || !strcmp(output_format_option, "trace"))
        conf.output_format = RDT_OUTPUT_TRACE;
    else if (!strcmp(output_format_option, "SQL") || !strcmp(output_format_option, "sql"))
        conf.output_format = RDT_OUTPUT_SQL;
    else if (!strcmp(output_format_option, "both"))
        conf.output_format = RDT_OUTPUT_BOTH;
    else
        error("Unknown format type: \"%s\"\n", output_format_option);

    //Rprintf("output_format_option=%s->%i\n", output_format_option,output_format);

    const char *output_type_option = get_string(get_named_list_element(options, "output"));
    if (output_type_option == NULL || !strcmp(output_type_option, "R") || !strcmp(output_type_option, "r"))
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

    //Rprintf("output_type_option=%s->%i\n", output_type_option,output_type);

    SEXP pretty_print_option = get_named_list_element(options, "pretty.print");
    if (pretty_print_option == NULL || TYPEOF(pretty_print_option) == NILSXP)
        conf.pretty_print = true;
    else
        conf.pretty_print = LOGICAL(pretty_print_option)[0] == TRUE;
    //Rprintf("pretty_print_option=%p->%i\n", (pretty_print_option), pretty_print);

    SEXP indent_width_option = get_named_list_element(options, "indent.width");
    if (indent_width_option != NULL && TYPEOF(indent_width_option) != NILSXP)
        if (TYPEOF(indent_width_option) == REALSXP)
            conf.indent_width = (int) *REAL(indent_width_option);
    //Rprintf("indent_width_option=%p->%i\n", indent_width_option, indent_width);

    SEXP overwrite_option = get_named_list_element(options, "overwrite");
    if (overwrite_option == NULL || TYPEOF(overwrite_option) == NILSXP)
        conf.overwrite = false;
    else
        conf.overwrite = LOGICAL(overwrite_option)[0] == TRUE;
    //Rprintf("overwrite_option=%p->%i\n", (overwrite_option), overwrite);

    SEXP synthetic_call_id_option = get_named_list_element(options, "synthetic.call.id");
    if (synthetic_call_id_option == NULL || TYPEOF(synthetic_call_id_option) == NILSXP)
        conf.call_id_use_ptr_fmt = false;
    else
        conf.call_id_use_ptr_fmt = LOGICAL(synthetic_call_id_option)[0] == FALSE;
    //Rprintf("call_id_use_ptr_fmt=%p->%i\n", (synthetic_call_id_option), call_id_use_ptr_fmt);

    return conf;
}

rdt_handler *setup_promise_tracing(SEXP options) {
    tracer_conf_t new_conf = get_config_from_R_options(options);
    tracer_conf.update(new_conf);

    if (tracer_conf.output_type != RDT_SQLITE && tracer_conf.output_type != RDT_R_PRINT_AND_SQLITE) {
        output = fopen(tracer_conf.filename, tracer_conf.overwrite ? "w" : "a");
        if (!output) {
            error("Unable to open %s: %s\n", tracer_conf.filename, strerror(errno));
            return NULL;
        }
    }

//    THIS IS DONE IN tracer_state().start_pass() (called from trace_promises_begin()) if the overwrite flag is set
//    call_id_counter = 0;
//    already_inserted_functions.clear();
//
//    if(tracer_conf.overwrite && (tracer_conf.output_type == RDT_SQLITE || tracer_conf.output_type == RDT_R_PRINT_AND_SQLITE)) {
//        remove(filename); // I wouldn't do this. Overwrite happens automatically when you actually open an existing file with "w".
//        argument_id_sequence = 0;
//        argument_ids.clear();
//    }

    if (tracer_conf.output_type == RDT_SQLITE || tracer_conf.output_type == RDT_R_PRINT_AND_SQLITE)
        rdt_init_sqlite(tracer_conf.filename);

    rdt_handler *h = (rdt_handler *)  malloc(sizeof(rdt_handler));
    memcpy(h, &trace_promises_rdt_handler, sizeof(rdt_handler));

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
    if (tracer_conf.output_type == RDT_SQLITE || tracer_conf.output_type == RDT_R_PRINT_AND_SQLITE)
        rdt_close_sqlite();
}
