#include <vector>
#include <string>
#include <stack>
#include <unordered_map>

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

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

// If defined printout will include increasing indents showing function calls.
#define RDT_PROMISES_INDENT
#define TAB_WIDTH 4

// Use generated call IDs instead of function env addresses
#define RDT_CALL_ID

typedef uintptr_t rid_t;
typedef int sid_t;

#define RID_INVALID -1

static FILE *output = NULL;
static uint64_t last = 0;
static uint64_t delta = 0;

static int indent;
static int call_id_counter;

// Function call stack (may be useful)
// Whenever R makes a function call, we generate a function ID and store that ID on top of the stack
// so that we know where we are (e.g. when printing function ID at function_exit hook)


static stack<rid_t, vector<rid_t>> fun_stack;

#ifdef RDT_CALL_ID
#define CALL_ID_FMT "%d"
static stack<rid_t, vector<rid_t>> curr_env_stack;
#else
#define CALL_ID_FMT "%#x"
#endif

// Map from promise IDs to call IDs
static unordered_map<rid_t, rid_t> promise_origin;

// XXX probably remove
static inline void p_print(const char *type, const char *loc, const char *name) {
    fprintf(output,
#ifdef RDT_PROMISES_INDENT
            "--%*s%s loc(%s) %s\n",
            indent,
            "",
#else
            "--%s loc(%s) %s\n",
#endif

            //delta,
            type,
            CHKSTR(loc),
            CHKSTR(name));
}

static inline void print_builtin(const char *type, const char *loc, const char *name, rid_t id) {
    fprintf(output,
#ifdef RDT_PROMISES_INDENT
            "--%*s%s loc(%s) function(%s=%#x)\n",
            indent,
            "",
#else
            "%s loc(%s) function(%s=%#x)\n",
#endif
            //delta,
            type,
            CHKSTR(loc),
            CHKSTR(name),
            id);
}

static inline void print_promise(const char *type, const char *loc, const char *name, rid_t id, rid_t call_id) {
    fprintf(output,
#ifdef RDT_PROMISES_INDENT
            "--%*s%s loc(%s) promise(%s=%#x) from_call(" CALL_ID_FMT ")\n",
            indent,
            "",
#else
            "%s loc(%s) promise(%s=%#x) from_call(" CALL_ID_FMT ")\n",
#endif
            //delta,
            type,
            CHKSTR(loc),
            CHKSTR(name),
            id,
            call_id);
}

static inline void mk_sql_promise() {

}

static inline void mk_sql_builtin() {

}

static inline string wrap_nullable_string(const char* s) {
    return s == NULL ? "NULL" : "'" + string(s) + "'";
}

static inline string wrap_string(string s) {
    return "'" + string(s) + "'";
}

static unordered_set<rid_t> already_inserted_functions;
static sid_t argument_id_sequence = 0;
static inline void mk_sql_function(rid_t function_id, vector<string> const& arguments, vector<sid_t> const& argument_ids, const char* location, const char* definition) {
    // Don't generate anything if one was previously generated.
    if(!already_inserted_functions.count(function_id)) {
        // Generate `functions' update containing function definition.
        Rprintf("insert into functions values (%#x,%s,%s);\n",
                function_id,
                wrap_nullable_string(location).c_str(),
                wrap_nullable_string(definition).c_str());

        // Generate `arguments' update wrt function above.
        if (arguments.size() > 0) {
            Rprintf("insert into arguments select");

            int index = 0;
            for (auto argument : arguments) {
                if (index)
                    Rprintf("\n            union all select");
                    //Rprintf(" union all select");
                Rprintf(" %i,%s,%i,%#x", argument_ids[index], wrap_string(argument).c_str(), index, function_id);
                index++;
            }
            Rprintf(";\n");
        }
        already_inserted_functions.insert(function_id);
    }
}

static inline void mk_sql_function_call(rid_t call_id, rid_t call_ptr, const char *name, const char* location, int call_type, rid_t function_id) {
    Rprintf("insert into calls values (%i,%#x,%s,%s,%i,%#x);\n",
            call_id,
            call_ptr,
            wrap_nullable_string(name).c_str(),
            wrap_nullable_string(location).c_str(),
            call_type,
            function_id
    );
}

static inline void mk_sql_promises(vector<rid_t> promises, rid_t call_id, vector<sid_t> argument_ids) {
    if (promises.size() > 0) {
        Rprintf("insert into promises  select");
        int index = 0;
        for (auto promise : promises) {
            if (index)
                Rprintf("\n            union all select");
                //Rprintf(" union all select");
            Rprintf(" %#x,%i,%i", promise, call_id, argument_ids[index]);
            index++;
        }
        Rprintf(";\n");
    }
}

static inline void mk_sql_promise_evaluation(int event_type, rid_t promise_id, rid_t call_id) {
    Rprintf("insert into promise_evaluations values ($next_id,%#x,%#x,%i);\n",
        event_type,
        promise_id,
        call_id
    );
}

static string concat_arguments(vector<string> const& arguments, /*const char **default_values, const char **promises,*/ int arguments_length) {
    string argument_string = "";

    for (int i=0; i<arguments_length; i++) {
        if (i)
            argument_string += ", ";

        argument_string += arguments[i];
//        argument_string = strcat(argument_string, promises[i]);
//
//        if (default_values[i] != NULL) {
//            argument_string = strcat(argument_string, "=");
//            argument_string = strcat(argument_string, default_values[i]);
//        }
    }

    return argument_string;
}

static string concat_promises(vector<string> const& arguments, /*const char **default_values,*/ vector<rid_t> const& promises, int arguments_length) {
    string promises_string = "";

    for (int i=0; i<arguments_length; i++) {
        if (i)
            promises_string += ", ";

        promises_string += arguments[i];
        promises_string += " = ";
        char * prom_id_str;
        asprintf(&prom_id_str, "%#x", promises[i]);
        promises_string += prom_id_str;
        free(prom_id_str);

        //if (default_values[i] != NULL) {
        //  promises_string = strcat(promises_string, "=");
        //  promises_string = strcat(promises_string, default_values[i]);
        //}
    }

    //Rprintf("String %s (%i vs %i)", promises_string, strlen(promises_string), (sizeof(char) * (characters + 1 * (arguments_length - 1) + 1)));

    return promises_string;
}

static inline void print_function(const char *type, const char *loc, const char *name, rid_t function_id, rid_t call_id, vector<string> const& arguments, /*const char **default_values,*/ vector<rid_t> const& promises, const int arguments_num) {
    string argument_string = concat_arguments(arguments, /* default_values, promises, */ arguments_num);
    string promises_string = concat_promises(arguments, /* default_values,*/ promises, arguments_num);

    fprintf(output,
#ifdef RDT_PROMISES_INDENT
        "--%*s%s loc(%s) call(" CALL_ID_FMT ") function(%s=%#x) params(%s) promises(%s)\n",
        indent, // http://stackoverflow.com/a/9448093/6846474
        "",
#else
        "--%s loc(%s) call(" CALL_ID_FMT ") function(%s=%#x) params(%s) promises(%s)\n",
#endif
        type,
        CHKSTR(loc),
        call_id,
        CHKSTR(name),
        function_id,
        argument_string.c_str(),
        promises_string.c_str()
    );
}

static inline void print_unwind(const char *type, rid_t call_id) {
    fprintf(output,
#ifdef RDT_PROMISES_INDENT
            "--%*s%s unwind call(" CALL_ID_FMT ")\n",
            indent,
            "",
#else
            "--%s unwind call(" CALL_ID_FMT ")\n",
#endif
            type,
            call_id
    );
}

//static inline void print_function_bare(const char *type, const char *loc, const char *name, const char *function_id, const char **arguments, int arguments_num) {
//#ifdef RDT_PROMISES_INDENT
//    char *indent_string = mk_indent();
//#endif
//
//    char *argument_string = concat_arguments(arguments, /* default_values, promises, */ arguments_num);
//    //char *promises_string = concat_promises(arguments, /* default_values,*/ promises, arguments_num);
//
//    fprintf(output,
//#ifdef RDT_PROMISES_INDENT
//            "%s%s loc(%s) function(%s=%s) params(%s)\n",
//            indent_string,
//#else
//            "%s loc(%s) function(%s=%s) params(%s)\n",
//#endif
//            type,
//            CHKSTR(loc),
//            CHKSTR(name),
//            CHKSTR(function_id),
//            argument_string
//            //promises_string
//    );
//
//#ifdef RDT_PROMISES_INDENT
//    if (indent_string)
//        free(indent_string);
//#endif
//    if (argument_string)
//        free(argument_string);
//    //if (promises_string)
//    //    free(promises_string);
//}

// TODO remove
static inline void compute_delta() {
    delta = (timestamp() - last) / 1000;
}

// ??? can we get metadata about the program we're analysing in here?
static void trace_promises_begin() {
    indent = 0;
    call_id_counter = 0;

    output = fopen("trace.sql", "w");

    //fprintf(output, "TYPE,LOCATION,NAME\n");
    //fflush(output);

    last = timestamp();
}
static void trace_promises_end() {
    promise_origin.clear();
    fclose(output);
}

static inline int count_elements(SEXP list) {
    int counter = 0;
    SEXP tmp = list;
    for (; tmp != R_NilValue; counter++)
        tmp = CDR(tmp);
    return counter;
}

//static inline char *trim_string(char *str) {
//    if (str == NULL)
//        return str;
//
//    int offset_bow = 0, offset_aft = 0;
//    char *aft = str + strlen(str) - 1;
//
//    for (; isspace((unsigned char) *(str + offset_bow)); offset_bow++);
//    for (; isspace((unsigned char) *(aft + offset_aft)); offset_aft--);
//
//    char *ret = malloc(sizeof(char *) * offset_aft - offset_bow + 1);
//    int ri = 0;
//    for (int si = offset_bow; si < offset_aft; si++, ri++)
//        ret[ri] = str[si];
//    ret[ri] = '\0';
//
//    return ret;
//}

//static inline char **strings_of_STRSXP(SEXP str, Rboolean flatten) {
//    // Currently I just want to handle a specific case here, so I'll return NULL for everything else.
//    if (TYPEOF(str) != STRSXP)
//        return NULL;
//
//    int size = XLENGTH(str); //count_elements(str);
//
//    char **strings = malloc((sizeof(char *) * size));
//    for (int i = 0; i < size; i++) {
//        Rprintf(">-----------------------[%d]\n", i);
//
//        strings[i] = strdup(CHAR(STRING_ELT(str, i)));
//
//        Rprintf("<-----------------------[%d] %s\n", i, strings[i]);
//    }
//}

//static inline char *flatten(char *str) {
//    int size = strlen(str);
//    for (int i = 0; i < size; i++)
//        if (str[i] == '\n') {
//            if (i)
//                if (str[i-1] == '{') {
//                    str[i] = ' ';
//                    continue;
//                }
//            if (i < size - 1)
//                if (str[i+1] == '}') {
//                    str[i] = ' ';
//                    continue;
//                }
//            str[i] = ';';
//        }
//    return str;
//}
//
//static inline char *remove_redundant_spaces(char *str) {
//    char *ret = malloc(sizeof(char) * (strlen(str) + 1));
//    int ret_size = 0;
//    int prec_is_space = 0;
//    for (int i = 0; str[i] != '\0'; i++)
//        if (str[i] == ' ' || str[i] == '\t') {
//            if (prec_is_space)
//                continue;
//            prec_is_space = 1;
//            ret[ret_size++] = ' ';
//        } else {
//            prec_is_space = 0;
//            ret[ret_size++] = str[i];
//        }
//    ret[ret_size] = '\0';
//    return ret;
//}

// TODO proper SEXP hashmap

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
#ifdef RDT_PROMISES_INDENT
        indent -= TAB_WIDTH;
#endif
        print_unwind("<=", call_id);
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

static inline int get_arguments(SEXP op, SEXP rho, vector<string> & arguments, vector<sid_t> & argument_ids, /*char ***return_default_values,*/ vector<rid_t> & promises) {
    SEXP formals = FORMALS(op);

    int argument_count = count_elements(formals);

    for (int i=0; i<argument_count; i++, formals = CDR(formals)) {
        // Retrieve the argument name.
        SEXP argument_expression = TAG(formals);
        arguments.push_back(get_name(argument_expression));
        argument_ids.push_back(++argument_id_sequence);

        // FIXME dot-dot-dot

        // Retrieve the default expression for the argument.
        // SEXP default_value_expression = CAR(formals);
        // if (default_value_expression != R_MissingArg) {
        //     SEXP deparsed_expression = deparse1line(default_value_expression, FALSE);
        //     // deparsed_expression has everything we need, but is formatted for display on console, so we de-prettify
        //     // it.
        //     char *flat_code = flatten(strdup(CHAR(STRING_ELT(deparsed_expression, 0))));
        //     default_values[i] = remove_redundant_spaces(flat_code);
        //     free(flat_code);
        // } else
        //     default_values[i] = NULL;

        // Retrieve the promise for the argument.
        // The call SEXP only contains AST to find the actual argument value, we need to search the environment.
        SEXP promise_expression = get_promise(argument_expression, rho);
        //asprintf(&promises[i], "[%p]", promise_expression);
        promises.push_back(get_promise_id(promise_expression));
        //Rprintf("promise=%s\n",promises[i]);
    }

    return argument_count;
}



// Triggggerrredd when entering function evaluation.
// TODO: function name, unique function identifier, arguments and their order, promises
// ??? where are promises created? and do we care?
// ??? will address of funciton change? garbage collector?
static void trace_promises_function_entry(const SEXP call, const SEXP op, const SEXP rho) {
    compute_delta();

    const char *type = is_byte_compiled(call) ? "=> bcod" : "=> func";
    int call_type = is_byte_compiled(call) ? 1 : 0;
    const char *name = get_name(call);
    const char *ns = get_ns_name(op);
    rid_t fn_id = get_function_id(op);
    rid_t call_ptr = get_sexp_address(op);
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

    vector<string> arguments;
    vector<sid_t> argument_ids;
    //char **default_values;
    vector<rid_t> promises;
    int argument_count;

    argument_count = get_arguments(op, rho, arguments, argument_ids,/*&default_values,*/ promises);
    print_function(type, loc, fqfn, fn_id, call_id, /*(const char **)*/arguments, /*default_values,*/ /*(const char **)*/ promises, argument_count);

    mk_sql_function(fn_id, arguments, argument_ids, loc, NULL);
    mk_sql_function_call(call_id, call_ptr, name, loc, call_type, fn_id);
    mk_sql_promises(promises, call_id, argument_ids);

    #ifdef RDT_PROMISES_INDENT
    indent += TAB_WIDTH;
    #endif

    // Associate promises with call ID
    for (auto p : promises) {
        promise_origin[p] = call_id;
    }

    if (loc)
        free(loc);
    if (fqfn)
        free(fqfn);

    //Rprintf("<o.o<\n");

    //if (default_values)
    //    free(default_values);
    //Rprintf("^o.o^\n");
    //Rprintf(">o.o>\n");

    last = timestamp();
}

static void trace_promises_function_exit(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
    compute_delta();

    #ifdef RDT_PROMISES_INDENT
    indent -= TAB_WIDTH;
    #endif

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

    vector<string> arguments;
    vector<sid_t> argument_ids;
    //char **default_values;
    vector<rid_t> promises;
    int argument_count;

    argument_count = get_arguments(op, rho, arguments, argument_ids, /*&default_values,*/ promises);
    print_function(type, loc, name, fn_id, call_id, arguments, /*default_values,*/ promises, argument_count);

    // Pop current function ID
    fun_stack.pop();
#ifdef RDT_CALL_ID
    curr_env_stack.pop();
#endif

    if (loc)
        free(loc);
    if (fqfn)
        free(fqfn);

    //Rprintf("<o.o<\n");

    //if (default_values)
    //    free(default_values);
    //Rprintf("^o.o^\n");

    //Rprintf(">o.o>\n");

    last = timestamp();
}

// XXX Probably don't need this?
static void trace_promises_builtin_entry(const SEXP call, const SEXP op, const SEXP rho) {
    compute_delta();

    const char *name = get_name(call);
    rid_t fn_id = get_function_id(op);

    rid_t call_ptr = get_sexp_address(call);
#ifdef RDT_CALL_ID
    rid_t call_id = make_funcall_id(call);
#endif

    print_builtin("=> b-in", NULL, name, fn_id);

    vector<string> arguments;
    vector<sid_t> argument_ids;
    //vector<rid_t> promises;

    mk_sql_function(fn_id, arguments, argument_ids, NULL, NULL);
    mk_sql_function_call(call_id, call_ptr, name, NULL, 1, fn_id);
    //mk_sql_promises(promises, call_id, argument_ids);

    //R_inspect(call);

    last = timestamp();
}

static void trace_promises_builtin_exit(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
    compute_delta();

    const char *name = get_name(call);
    rid_t id = get_function_id(op);

    print_builtin("<= b-in", NULL, name, id);

    last = timestamp();
}

// Promise is being used inside a function body for the first time.
// TODO name of promise, expression inside promise, value evaluated if available, (in the long term) connected to a function
static void trace_promises_force_promise_entry(const SEXP symbol, const SEXP rho) {
    compute_delta();

    const char *name = get_name(symbol);

    SEXP promise_expression = get_promise(symbol, rho);
    rid_t id = get_promise_id(promise_expression);
    rid_t call_id = promise_origin[id];

    print_promise("=> prom", NULL, name, id, call_id);

    mk_sql_promise_evaluation(0xf, id, call_id);

    last = timestamp();
}

static void trace_promises_force_promise_exit(const SEXP symbol, const SEXP rho, const SEXP val) {
    compute_delta();

    const char *name = get_name(symbol);

    SEXP promise_expression = get_promise(symbol, rho);
    rid_t id = get_promise_id(promise_expression);
    rid_t call_id = promise_origin[id];

    print_promise("<= prom", NULL, name, id, call_id);

    last = timestamp();
}

static void trace_promises_promise_lookup(const SEXP symbol, const SEXP rho, const SEXP val) {
    compute_delta();

    const char *name = get_name(symbol);

    SEXP promise_expression = get_promise(symbol, rho);
    rid_t id = get_promise_id(promise_expression);
    rid_t call_id = promise_origin[id];

    print_promise("<> lkup", NULL, name, id, call_id);

    mk_sql_promise_evaluation(0x0, id, call_id);

    last = timestamp();
}

static void trace_promises_error(const SEXP call, const char* message) {
    compute_delta();

    char *call_str = NULL;
    char *loc = get_location(call);

    asprintf(&call_str, "\"%s\"", get_call(call));

    //p_print("error", NULL, call_str);

    if (loc) free(loc);
    if (call_str) free(call_str);

    last = timestamp();
}

static void trace_promises_vector_alloc(int sexptype, long length, long bytes, const char* srcref) {
    compute_delta();
    //p_print("vector-alloc", NULL, NULL);
    last = timestamp();
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
    compute_delta();
    //p_print("builtin-entry", NULL, "gc_internal");
    last = timestamp();
}

static void trace_promises_gc_exit(int gc_count, double vcells, double ncells) {
    compute_delta();
    //p_print("builtin-exit", NULL, "gc_internal");
    last = timestamp();
}

static void trace_promises_S3_generic_entry(const char *generic, const SEXP object) {
    compute_delta();

    //p_print("s3-generic-entry", NULL, generic);

    last = timestamp();
}

static void trace_promises_S3_generic_exit(const char *generic, const SEXP object, const SEXP retval) {
    compute_delta();

    //p_print("s3-generic-exit", NULL, generic);

    last = timestamp();
}

static void trace_promises_S3_dispatch_entry(const char *generic, const char *clazz, const SEXP method, const SEXP object) {
    compute_delta();

    //p_print("s3-dispatch-entry", NULL, get_name(method));

    last = timestamp();
}

static void trace_promises_S3_dispatch_exit(const char *generic, const char *clazz, const SEXP method, const SEXP object, const SEXP retval) {
    compute_delta();

    //p_print("s3-dispatch-exit", NULL, get_name(method));

    last = timestamp();
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
    const char *filename = get_string(get_named_list_element(options, "filename"));
    output = filename != NULL ? fopen(filename, "wt") : stderr;

    if (!output) {
        error("Unable to open %s: %s\n", filename, strerror(errno));
        return NULL;
    }

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

    last = 0;
    delta = 0;

    return h;
}
