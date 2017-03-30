//#ifdef HAVE_CONFIG_H
//# include <config.h>
//#endif
//#include <Defn.h>

#include <vector>
#include <string>
#include <stack>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <cstring>
#include <fstream>
#include <functional>

#include "rdt_register_hook.h"

#include "rdt_promises/tracer_conf.h"
#include "rdt_promises/tracer_output.h"
#include "rdt_promises/tracer_state.h"

#include "rdt.h"

using namespace std;

//static inline int count_elements(SEXP list) {
//    int counter = 0;
//    SEXP tmp = list;
//    for (; tmp != R_NilValue; counter++)
//        tmp = CDR(tmp);
//    return counter;
//}



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

    static void printEntryInfo(const SEXP call, const SEXP op, const SEXP rho, const char * msg) {
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
        rdt_print(RDT_OUTPUT_TRACE, {print_builtin(msg, NULL, name, fn_id, call_id)});

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

    // TODO retrieve arguments
    DECL_HOOK(builtin_entry)(const SEXP call, const SEXP op, const SEXP rho) {
        printEntryInfo(call, op, rho, "=> b-in");
    }

    DECL_HOOK(specialsxp_entry)(const SEXP call, const SEXP op, const SEXP rho) {
        printEntryInfo(call, op, rho, "=> specialsxp");
    }

    static void printExitInfo(const SEXP call, const SEXP op, const SEXP rho, const char * msg) {
        const char *name = get_name(call);
        fn_addr_t id = get_function_id(op);
        call_id_t call_id = STATE(fun_stack).top();

        STATE(fun_stack).pop();
#ifdef RDT_CALL_ID
        STATE(curr_env_stack).pop();
#endif
        if (tracer_conf.pretty_print)
            STATE(indent) -= tracer_conf.indent_width;

        rdt_print(RDT_OUTPUT_TRACE, {print_builtin(msg, NULL, name, id, call_id)});
    }

    DECL_HOOK(builtin_exit)(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
        printExitInfo(call, op, rho, "<= b-in");
    }

    DECL_HOOK(specialsxp_exit)(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
        printExitInfo(call, op, rho, "<= specialsxp");
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
            error("Unable to open %s: %s\n", tracer_conf.filename->c_str(), strerror(errno));
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
                        tr::specialsxp_entry,
                        tr::specialsxp_exit,
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
