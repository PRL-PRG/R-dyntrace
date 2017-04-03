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
//#include "rdt_promises/tracer_output.h"
#include "rdt_promises/tracer_state.h"

#include "rdt.h"
#include "rdt_promises/recorder.h"
#include "rdt_promises/trace_recorder.h"
#include "rdt_promises/sql_recorder.h"

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
template<typename Rec>
struct trace_promises {
    static Rec rec_impl;
    static recorder_t<Rec>& rec;

    // ??? can we get metadata about the program we're analysing in here?
    // TODO: also pass environment
    DECL_HOOK(begin)(const SEXP prom) {
        tracer_state().start_pass(prom);
    }

    DECL_HOOK(end)() {
        tracer_state().finish_pass();

        // FIXME
//        if (output) {
//            fclose(output);
//            output = NULL;
//        }
    }

    // Triggered when entering function evaluation.
    DECL_HOOK(function_entry)(const SEXP call, const SEXP op, const SEXP rho) {
        call_info_t info = rec.function_entry_get_info(call, op, rho);

        // Push function ID on function stack
        STATE(fun_stack).push(info.call_id);
#ifdef RDT_CALL_ID
        STATE(curr_env_stack).push(info.call_ptr);
#endif

        rec.function_entry_process(info);

        auto & fresh_promises = STATE(fresh_promises);
        // Associate promises with call ID
        for (auto arg_ref : info.arguments.all()) {
            const arg_t & argument = arg_ref.get();
            auto & promise = get<2>(argument);
            auto it = fresh_promises.find(promise);

            if (it != fresh_promises.end()) {
                STATE(promise_origin)[promise] = info.call_id;
                fresh_promises.erase(it);
            }
        }
    }

    DECL_HOOK(function_exit)(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {

        call_info_t info = rec.function_exit_get_info(call, op, rho);
        rec.function_exit_process(info);

        // Pop current function ID
        STATE(fun_stack).pop();
#ifdef RDT_CALL_ID
        STATE(curr_env_stack).pop();
#endif
    }

    // TODO retrieve arguments
    DECL_HOOK(builtin_entry)(const SEXP call, const SEXP op, const SEXP rho) {
        call_info_t info = rec.builtin_entry_get_info(call, op, rho);
        rec.builtin_entry_process(info);

        STATE(fun_stack).push(info.call_id);
#ifdef RDT_CALL_ID
        STATE(curr_env_stack).push(info.call_ptr | 1);
#endif
    }

    DECL_HOOK(builtin_exit)(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
        call_info_t info = rec.builtin_exit_get_info(call, op, rho);
        rec.builtin_exit_process(info);

        STATE(fun_stack).pop();
#ifdef RDT_CALL_ID
        STATE(curr_env_stack).pop();
#endif
    }

    DECL_HOOK(promise_created)(const SEXP prom) {
        prom_id_t prom_id = make_promise_id(prom);
        STATE(fresh_promises).insert(prom_id);

        rec.promise_created_process(prom_id);
    }

    // Promise is being used inside a function body for the first time.
    DECL_HOOK(force_promise_entry)(const SEXP symbol, const SEXP rho) {
        prom_info_t info = rec.force_promise_entry_get_info(symbol, rho);
        rec.force_promise_entry_process(info);
    }

    DECL_HOOK(force_promise_exit)(const SEXP symbol, const SEXP rho, const SEXP val) {
        prom_info_t info = rec.force_promise_exit_get_info(symbol, rho);
        rec.force_promise_exit_process(info);
    }

    DECL_HOOK(promise_lookup)(const SEXP symbol, const SEXP rho, const SEXP val) {
        prom_info_t info = rec.promise_lookup_get_info(symbol, rho);
        rec.promise_lookup_process(info);
    }

    DECL_HOOK(error)(const SEXP call, const char* message) {
        char *call_str = NULL;
        char *loc = get_location(call);

        // FIXME: Shouldn't we use `call_str`?
        asprintf(&call_str, "\"%s\"", get_call(call));

        if (loc) free(loc);
        if (call_str) free(call_str);
    }

    DECL_HOOK(vector_alloc)(int sexptype, long length, long bytes, const char* srcref) {
    }

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
};

// Static member initialization
template<typename Rec>
Rec trace_promises<Rec>::rec_impl;

template<typename Rec>
recorder_t<Rec>& trace_promises<Rec>::rec = rec_impl;

static bool file_exists(const string & fname) {
    ifstream f(fname);
    return f.good();
}

template<typename Rec>
rdt_handler register_hooks_with() {
    // Because Rec is an unknown type (until template instantiation) we have to
    // explicitly tell the compiler that trace_promises<Rec>::hook_name is also a type
#define tp typename tr
    return REGISTER_HOOKS(trace_promises<Rec>,
                        tp::begin,
                        tp::end,
                        tp::function_entry,
                        tp::function_exit,
                        tp::builtin_entry,
                        tp::builtin_exit,
                        tp::force_promise_entry,
                        tp::force_promise_exit,
                        tp::promise_lookup,
                        tp::error,
                        tp::vector_alloc,
                        tp::gc_promise_unmarked,
                        tp::jump_ctxt,
                        tp::promise_created);
#undef tp
}

rdt_handler *setup_promise_tracing(SEXP options) {
    tracer_conf_t new_conf = get_config_from_R_options(options);
    tracer_conf.update(new_conf);


    // TODO: can we move these into `begin` hook or possibly trace/sql_recorder implementations?
//    if (tracer_conf.output_type != OutputType::RDT_SQLITE && tracer_conf.output_type != OutputType::RDT_R_PRINT_AND_SQLITE) {
//        output = fopen(tracer_conf.filename->c_str(), tracer_conf.overwrite ? "w" : "a");
//        if (!output) {
//            error("Unable to open %s: %s\n", tracer_conf.filename, strerror(errno));
//            return NULL;
//        }
//    }
    // FIXME moved to multiplexer::init

//    THIS IS DONE IN tracer_state().start_pass() (called from trace_promises_begin()) if the overwrite flag is set
//    call_id_counter = 0;
//    already_inserted_functions.clear();

//    if (tracer_conf.output_type == OutputType::RDT_SQLITE || tracer_conf.output_type == OutputType::RDT_R_PRINT_AND_SQLITE) {
//        if(tracer_conf.overwrite) {
//            if (file_exists(tracer_conf.filename)) {
//                remove(tracer_conf.filename->c_str());
//            }
//        }
//        rdt_init_sqlite(tracer_conf.filename);
//    }
    // FIXME MOVED TO multiplexer::init

    // FIXME CALL FUNCTIONS FROM MULTIPLEXER INIT!!!!!

    if (tracer_conf.output_format != OutputFormat::RDT_OUTPUT_TRACE) {
        // rdt_configure_sqlite(); FIXME NEW API
        // FIXME ALSO CALL STH TO LOAD SCHEMA
        // rdt_begin_transaction(); FIXME NEW API
    }

    rdt_handler *h = (rdt_handler *) malloc(sizeof(rdt_handler));
    //memcpy(h, &trace_promises_rdt_handler, sizeof(rdt_handler));
    //*h = trace_promises_rdt_handler; // This actually does the same thing as memcpy
    if (tracer_conf.output_format == OutputFormat::RDT_OUTPUT_TRACE) {
        Rprintf("1\n");
        *h = register_hooks_with<trace_recorder_t>();
    }
    else if (tracer_conf.output_format == OutputFormat::RDT_OUTPUT_SQL || tracer_conf.output_format == OutputFormat::RDT_OUTPUT_COMPILED_SQLITE) {
        Rprintf("2\n");
        *h = register_hooks_with<sql_recorder_t>();
    }
    else { // RDT_OUTPUT_BOTH
        Rprintf("3\n");
        *h = register_hooks_with<compose<trace_recorder_t, sql_recorder_t>>();
    }

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
    //if (tracer_conf.output_format != OutputFormat::RDT_OUTPUT_TRACE)
        //rdt_commit_transaction(); //FIXME implement this using new functions

    //if (tracer_conf.output_type == OutputType::RDT_SQLITE || tracer_conf.output_type == OutputType::RDT_R_PRINT_AND_SQLITE)
        //rdt_close_sqlite(); //FIXME implement this using new functions
}
