#include <vector>
#include <string>
#include <stack>
#include <sstream>
#include <initializer_list>
#include <unordered_map>
#include <tuple>
#include <unordered_map>
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
static int indent;
static int call_id_counter;

// Function call stack (may be useful)
// Whenever R makes a function call, we generate a function ID and store that ID on top of the stack
// so that we know where we are (e.g. when printing function ID at function_exit hook)
static stack<rid_t, vector<rid_t>> fun_stack;

#ifdef RDT_CALL_ID
#define CALL_ID_FMT "%d"
static stack<rid_t, vector<rid_t>> curr_env_stack;
static bool call_id_use_ptr_fmt = false;
#else
#define CALL_ID_FMT "%#x"
static bool call_id_use_ptr_fmt = true;
#endif

// Map from promise IDs to call IDs
static unordered_map<rid_t, rid_t> promise_origin;

typedef vector<rid_t> prom_vec_t;
typedef tuple<string, sid_t, prom_vec_t> arg_t;

enum OutputFormat: char {RDT_OUTPUT_TRACE, RDT_OUTPUT_SQL, RDT_OUTPUT_BOTH};
enum Output: char {RDT_R_PRINT, RDT_FILE, RDT_SQLITE, RDT_R_PRINT_AND_SQLITE};

// TODO make these settable from Rdt(...)
static int output_type = RDT_R_PRINT_AND_SQLITE;
static int output_format = RDT_OUTPUT_BOTH;
static bool pretty_print = true;
static bool overwrite = false;
static int indent_width = 4;
//static bool synthetic_call_id = true;

static inline void prepend_prefix(stringstream *stream) {
    if (output_format == RDT_OUTPUT_TRACE) {
        if (pretty_print)
            (*stream) << string(indent, ' ');
    } else
        (*stream) << "-- ";
}

static inline void rdt_print(OutputFormat string_format, std::initializer_list<string> strings) {
    if (output_format != RDT_OUTPUT_BOTH && string_format != output_format)
        return;

    switch (output_type) {
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

            if (output_type == RDT_R_PRINT_AND_SQLITE)
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

    if (pretty_print && (output_format == RDT_OUTPUT_TRACE))
        stream << string(indent, ' ');

    stream << type << " "
           << "loc(" << CHKSTR(loc) << ") "
           << "fun(" << CHKSTR(name) << "=" << hex << id << ")\n";

    return stream.str();
}

static inline string print_promise(const char *type, const char *loc, const char *name, rid_t id, rid_t in_call_id, rid_t from_call_id) {
    stringstream stream;
    prepend_prefix(&stream);

    if (pretty_print && (output_format == RDT_OUTPUT_TRACE))
        stream << string(indent, ' ');

    stream << type << " "
           << "loc(" << CHKSTR(loc) << ") "
           << "prom(" << CHKSTR(name) << "=" << hex << id << ") ";
    stream << "in(" << (call_id_use_ptr_fmt ? hex : dec) << in_call_id << ") ";
    stream << "from(" << (call_id_use_ptr_fmt ? hex : dec) << from_call_id << ")\n";

    return stream.str();
}

static inline string wrap_nullable_string(const char* s) {
    return s == NULL ? "NULL" : "'" + string(s) + "'";
}

static inline string wrap_string(string s) {
    return "'" + string(s) + "'";
}

static unordered_set<rid_t> already_inserted_functions;
static sid_t argument_id_sequence = 0;
static inline string mk_sql_function(rid_t function_id, vector<arg_t> const& arguments, const char* location, const char* definition) {
    stringstream stream;
    // Don't generate anything if one was previously generated.
    if(!already_inserted_functions.count(function_id)) {
        // Generate `functions' update containing function definition.
        stream << "insert into functions values ("
               << "0x" << hex << function_id << ",";
        stream << wrap_nullable_string(location) << ","
               << wrap_nullable_string(definition) <<
               ");\n";

        // Generate `arguments' update wrt function above.
        if (arguments.size() > 0) {
            stream << "insert into arguments select";

            int index = 0;
            for (auto argument : arguments) {
                if (index)
                    stream << (pretty_print ? "\n            " : " ") << "union all select";

                stream << " "
                       << dec << get<1>(argument) << ","
                       << wrap_string(get<0>(argument)) << ","
                       << index << ","
                       << "0x" << hex << function_id;

                index++;
            }
            stream << ";\n";
        }
        already_inserted_functions.insert(function_id);
    }
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

static inline string mk_sql_promises(vector<arg_t> arguments, rid_t call_id) {
    std::stringstream stream;
    if (arguments.size() > 0) {
        stream << (pretty_print ? "insert into promises  select" : "insert into promises select");
        int index = 0;
        for (auto & argument : arguments) {
            sid_t arg_id = get<1>(argument);
            for (auto promise : get<2>(argument)) {
                if (index)
                    stream << (pretty_print ? "\n            " : " ") << "union all select";

                stream << " "
                       << "0x" << hex << promise << ","
                       << dec << call_id << ","
                       << dec << arg_id;
                index++;
            }
        }
        stream << ";\n";
    }
    return stream.str();
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


static inline string print_function(const char *type, const char *loc, const char *name, rid_t function_id, rid_t call_id, vector<arg_t> const& arguments, const int arguments_num) {
    stringstream stream;
    prepend_prefix(&stream);

    if (pretty_print && (output_format == RDT_OUTPUT_TRACE))
        stream << string(indent, ' ');

    stream << type << " "
           << "loc(" << CHKSTR(loc) << ") "
           << "call(" << (call_id_use_ptr_fmt ? hex : dec) << call_id << ") ";
    stream << "fun(" << CHKSTR(name) << "=" << hex << function_id << ") ";

    // print argument names and the promises bound to them
    stream << "arguments(";
    int i = 0;
    for (auto & a : arguments) {
        const prom_vec_t & p = get<2>(a);
        //fprintf(output, "%s=%#x", get<0>(a).c_str(), p[0]);
        stream << get<0>(a).c_str() << "=" << p[0];

        // iterate from second bound value if there is any
        // and produce comma separated list of promises
        for (auto it = ++p.begin(); it != p.end(); it++)
//            fprintf(output, ",%#x", *it);
            stream << "," << *it;
        if (i < arguments_num - 1)
            stream << ";";
            //fprintf(output, ";"); // semicolon separates individual args
        ++i;
    }

    stream << ")\n";
    //fprintf(output, ")\n");

    return stream.str();
}

static inline string print_unwind(const char *type, rid_t call_id) {
    stringstream stream;
    prepend_prefix(&stream);
    stream << type << " "
           << "unwind(" << (call_id_use_ptr_fmt ? hex : dec) << call_id << ")\n";
    return stream.str();
}

// ??? can we get metadata about the program we're analysing in here?
static void trace_promises_begin() {
    indent = 0;

    // We have to make sure the stack is not empty
    // when referring to the promise created by call to Rdt.
    // This is just a dummy call and environment.
    fun_stack.push(0);
#ifdef RDT_CALL_ID
    curr_env_stack.push(0);
#endif
}
static void trace_promises_end() {
    fun_stack.pop();
#ifdef RDT_CALL_ID
    curr_env_stack.pop();
#endif

    promise_origin.clear();

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

static inline rid_t get_sexp_address(SEXP e) {
    return (rid_t)e;
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

    return ++call_id_counter;
}
#else
static inline rid_t make_funcall_id(SEXP fn_env) {
    assert(fn_env != NULL);
    return get_sexp_address(fn_env);
}
#endif

// When doing longjump (exception thrown, etc.) this function gets the target environment
// and unwinds function call stack until that environment is on top. It also fixes indentation.
static inline void adjust_fun_stack(SEXP rho) {
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

        if (pretty_print)
            indent -= indent_width;
        rdt_print(RDT_OUTPUT_TRACE, {print_unwind("<=", call_id)});
    }
}

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

static inline int get_arguments(SEXP op, SEXP rho, vector<arg_t> & arguments) {
    SEXP formals = FORMALS(op);

    int argument_count = count_elements(formals);

    for (int i=0; i<argument_count; i++, formals = CDR(formals)) {
        // Retrieve the argument name.
        SEXP argument_expression = TAG(formals);

        arg_t arg;
        string & arg_name = get<0>(arg);
        sid_t & arg_id = get<1>(arg);
        prom_vec_t & arg_prom_vec = get<2>(arg);

        arg_name = get_name(argument_expression);
        //if (pair(arg_name,function_id) in argument_id_map)
        //arg_id <- argument_id_map(pair(arg_name, function_id))
        arg_id = ++argument_id_sequence; //FIXME?

        // Retrieve the promise for the argument.
        // The call SEXP only contains AST to find the actual argument value, we need to search the environment.
        SEXP promise_expression = get_promise(argument_expression, rho);

        if (TYPEOF(promise_expression) == DOTSXP) {
            for(SEXP dots = promise_expression; dots != R_NilValue; dots = CDR(dots)) {
                arg_prom_vec.push_back(get_promise_id(CAR(dots)));
            }
        }
        else {
            arg_prom_vec.push_back(get_promise_id(promise_expression));
        }
        arguments.push_back(arg);
    }

    return argument_count;
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
    fun_stack.push(call_id);
#ifdef RDT_CALL_ID
    curr_env_stack.push(get_sexp_address(rho));
#endif

    vector<arg_t> arguments;
    int argument_count;

    argument_count = get_arguments(op, rho, arguments);
    // TODO link with mk_sql
    // TODO rename to reflect non-printing nature
    rdt_print(RDT_OUTPUT_TRACE, {print_function(type, loc, fqfn, fn_id, call_id, arguments, argument_count)});

    rdt_print(RDT_OUTPUT_SQL, {mk_sql_function(fn_id, arguments, loc, NULL),
               mk_sql_function_call(call_id, call_ptr, name, loc, call_type, fn_id),
               mk_sql_promises(arguments, call_id)});

    if (pretty_print)
        indent += indent_width;

    // Associate promises with call ID
    for (auto & a : arguments) {
        auto & promises = get<2>(a);
        for (auto p : promises) {
            promise_origin[p] = call_id;
        }
    }

    if (loc)
        free(loc);
    if (fqfn)
        free(fqfn);
}

static void trace_promises_function_exit(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
    if (pretty_print)
        indent -= indent_width;

    const char *type = is_byte_compiled(call) ? "<= bcod" : "<= func";
    const char *name = get_name(call);
    const char *ns = get_ns_name(op);
    rid_t fn_id = get_function_id(op);
    rid_t call_id = fun_stack.top();
    char *loc = get_location(op);
    char *fqfn = NULL;

    if (ns) {
        asprintf(&fqfn, "%s::%s", ns, CHKSTR(name));
    } else {
        fqfn = name != NULL ? strdup(name) : NULL;
    }

    vector<arg_t> arguments;
    int argument_count;

    argument_count = get_arguments(op, rho, arguments);
    rdt_print(RDT_OUTPUT_TRACE, {print_function(type, loc, name, fn_id, call_id, arguments, argument_count)});

    // Pop current function ID
    fun_stack.pop();
#ifdef RDT_CALL_ID
    curr_env_stack.pop();
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

    vector<arg_t> arguments;

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
    rid_t in_call_id = fun_stack.top();
    rid_t from_call_id = promise_origin[id];

    // TODO: save in_call_id to db
    // in_call_id = current call
    rdt_print(RDT_OUTPUT_TRACE, {print_promise("=> prom", NULL, name, id, in_call_id, from_call_id)});
    rdt_print(RDT_OUTPUT_SQL, {mk_sql_promise_evaluation(RDT_FORCE_PROMISE, id, from_call_id)});

}

static void trace_promises_force_promise_exit(const SEXP symbol, const SEXP rho, const SEXP val) {
    const char *name = get_name(symbol);

    SEXP promise_expression = get_promise(symbol, rho);
    rid_t id = get_promise_id(promise_expression);
    rid_t in_call_id = fun_stack.top();
    rid_t from_call_id = promise_origin[id];

    rdt_print(RDT_OUTPUT_TRACE, {print_promise("<= prom", NULL, name, id, in_call_id, from_call_id)});
}

static void trace_promises_promise_lookup(const SEXP symbol, const SEXP rho, const SEXP val) {
    const char *name = get_name(symbol);

    SEXP promise_expression = get_promise(symbol, rho);
    rid_t id = get_promise_id(promise_expression);
    rid_t in_call_id = fun_stack.top();
    rid_t from_call_id = promise_origin[id];

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

    auto iter = promise_origin.find(id);
    if (iter != promise_origin.end()) {
        // If this is one of our traced promises,
        // delete it from origin map because it is ready to be GCed
        promise_origin.erase(iter);
        //Rprintf("Promise %#x deleted.\n", id);
    }
}

static void trace_promises_jump_ctxt(const SEXP rho, const SEXP val) {
    adjust_fun_stack(rho);
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

rdt_handler *setup_promise_tracing(SEXP options) {
    const char *filename = get_string(get_named_list_element(options, "path"));
    if (filename == NULL)
        filename = "trace.db";

    const char *output_format_option = get_string(get_named_list_element(options, "format"));
    if (output_format_option == NULL || !strcmp(output_format_option, "trace"))
        output_format = RDT_OUTPUT_TRACE;
    else if (!strcmp(output_format_option, "SQL") || !strcmp(output_format_option, "sql"))
        output_format = RDT_OUTPUT_SQL;
    else if (!strcmp(output_format_option, "both"))
        output_format = RDT_OUTPUT_BOTH;
    else
        error("Unknown format type: \"%s\"\n", output_format_option);

    //Rprintf("output_format_option=%s->%i\n", output_format_option,output_format);

    const char *output_type_option = get_string(get_named_list_element(options, "output"));
    if (output_type_option == NULL || !strcmp(output_type_option, "R") || !strcmp(output_type_option, "r"))
        output_type = RDT_R_PRINT;
    else if (!strcmp(output_type_option, "file"))
        output_type = RDT_FILE;
    else if (!strcmp(output_type_option, "DB") || !strcmp(output_type_option, "db"))
        output_type = RDT_SQLITE;
    else if (!strcmp(output_type_option, "R+DB") || !strcmp(output_type_option, "r+db") ||
        !strcmp(output_type_option, "DB+R") || !strcmp(output_type_option, "db+r"))
        output_type = RDT_R_PRINT_AND_SQLITE;
    else
        error("Unknown format type: \"%s\"\n", output_type_option);

    //Rprintf("output_type_option=%s->%i\n", output_type_option,output_type);

    SEXP pretty_print_option = get_named_list_element(options, "pretty.print");
    if (pretty_print_option == NULL || TYPEOF(pretty_print_option) == NILSXP)
        pretty_print = true;
    else
        pretty_print = LOGICAL(pretty_print_option)[0] == TRUE;
    //Rprintf("pretty_print_option=%p->%i\n", (pretty_print_option), pretty_print);

    SEXP indent_width_option = get_named_list_element(options, "indent.width");
    R_inspect(indent_width_option);
    if (indent_width_option != NULL && TYPEOF(indent_width_option) != NILSXP)
        if (TYPEOF(indent_width_option) == REALSXP)
            indent_width = (int) *REAL(indent_width_option);
    //Rprintf("indent_width_option=%p->%i\n", indent_width_option, indent_width);

    SEXP overwrite_option = get_named_list_element(options, "overwrite");
    if (overwrite_option == NULL || TYPEOF(overwrite_option) == NILSXP)
        overwrite = false;
    else
        overwrite = LOGICAL(overwrite_option)[0] == TRUE;
    //Rprintf("overwrite_option=%p->%i\n", (overwrite_option), overwrite);

    SEXP synthetic_call_id_option = get_named_list_element(options, "synthetic.call.id");
    if (synthetic_call_id_option == NULL || TYPEOF(synthetic_call_id_option) == NILSXP)
        call_id_use_ptr_fmt = false;
    else
        call_id_use_ptr_fmt = LOGICAL(synthetic_call_id_option)[0] == FALSE;
    //Rprintf("call_id_use_ptr_fmt=%p->%i\n", (synthetic_call_id_option), call_id_use_ptr_fmt);

    if (output_type != RDT_SQLITE && output_type != RDT_R_PRINT_AND_SQLITE) {
        output = fopen(filename, overwrite ? "w" : "wt"); // TODO options for wt?
        if (!output) {
            error("Unable to open %s: %s\n", filename, strerror(errno));
            return NULL;
        }
    }

    // XXX I moved this counter initialization here, so that several consecutive Rdt calls can be put into the same trace/DB/etc.
    // FIXME if not overwrite, then this should be some number, otherwise call IDs will get screwed up, right?
    call_id_counter = 0;
    already_inserted_functions.clear();

    if(overwrite && (output_type == RDT_SQLITE || output_type == RDT_R_PRINT_AND_SQLITE))
        remove(filename);

    if (output_type == RDT_SQLITE || output_type == RDT_R_PRINT_AND_SQLITE)
        rdt_init_sqlite(filename);

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
    if (output_type == RDT_SQLITE || output_type == RDT_R_PRINT_AND_SQLITE)
        rdt_close_sqlite();
}