//#ifdef HAVE_CONFIG_H
//# include <config.h>
//#endif
//#include <Defn.h>

#include <cstring>
#include <fstream>
#include <functional>
#include <sstream>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>


#include "tracer_conf.h"
//#include "rdt_promises/tracer_output.h"
#include "tracer_state.h"

#include "psql_recorder.h"
#include "rdt_promises.h"
#include "recorder.h"
#include "sql_recorder.h"
#include "trace_recorder.h"
#include <rdt.h>

using namespace std;

//static inline int count_elements(SEXP list) {
//    int counter = 0;
//    SEXP tmp = list;
//    for (; tmp != R_NilValue; counter++)
//        tmp = CDR(tmp);
//    return counter;
//}

// All the interpreter hooks go here
template<typename Rec>
struct trace_promises {
    static Rec rec_impl;
    static recorder_t<Rec>& rec;

    static void begin(const SEXP prom) {
        PROTECT(prom);

        int gc_enabled = gc_toggle_off();

        tracer_state().start_pass(prom);

        metadata_t metadata;
        //= rec.get_metadata_from_environment();
        rec.get_environment_metadata(metadata);
        rec.get_current_time_metadata(metadata, "START");

        rec.start_trace_process(metadata);

        gc_toggle_restore(gc_enabled);

        UNPROTECT(1);
    }

    static void end() {
        int gc_enabled = gc_toggle_off();

        tracer_state().finish_pass();

        metadata_t metadata;
        rec.get_current_time_metadata(metadata, "END");
        rec.finish_trace_process(metadata);


        if (!STATE(fun_stack).empty()) {
            Rprintf("Function stack is not balanced: %d remaining.\n", STATE(fun_stack).size());
            STATE(fun_stack).clear();
        }

        if (!STATE(full_stack).empty()) {
            Rprintf("Function/promise stack is not balanced: %d remaining.\n", STATE(full_stack).size());
            STATE(full_stack).clear();
        }

        gc_toggle_restore(gc_enabled);
    }

    // Triggered when entering function evaluation.
    static void function_entry(const SEXP call, const SEXP op, const SEXP rho) {
        int gc_enabled = gc_toggle_off();

        PROTECT(call);
        PROTECT(op);
        PROTECT(rho);

        closure_info_t info = rec.function_entry_get_info(call, op, rho);

        // Push function ID on function stack
        STATE(fun_stack).push_back(make_tuple(info.call_id, info.fn_id, info.fn_type));
        STATE(curr_env_stack).push(info.call_ptr);

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

        UNPROTECT(3);

        gc_toggle_restore(gc_enabled);
    }

    static void function_exit(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
        int gc_enabled = gc_toggle_off();

        PROTECT(call);
        PROTECT(op);
        PROTECT(rho);
        PROTECT(retval);

        closure_info_t info = rec.function_exit_get_info(call, op, rho);
        rec.function_exit_process(info);

        // Current function ID is popped in function_exit_get_info
        STATE(curr_env_stack).pop();

        UNPROTECT(4);

        gc_toggle_restore(gc_enabled);
    }

    static void print_entry_info(const SEXP call, const SEXP op, const SEXP rho, function_type fn_type) {
        int gc_enabled = gc_toggle_off();

        PROTECT(call);
        PROTECT(op);
        PROTECT(rho);

        builtin_info_t info = rec.builtin_entry_get_info(call, op, rho, fn_type);
        rec.builtin_entry_process(info);

        STATE(fun_stack).push_back(make_tuple(info.call_id, info.fn_id, info.fn_type));
        STATE(curr_env_stack).push(info.call_ptr | 1);

        UNPROTECT(3);

        gc_toggle_restore(gc_enabled);
    }

    static void builtin_entry(const SEXP call, const SEXP op, const SEXP rho) {
        int gc_enabled = gc_toggle_off();

        PROTECT(call);
        PROTECT(op);
        PROTECT(rho);

        function_type fn_type;
        if(TYPEOF(op)==BUILTINSXP)
            fn_type = (PRIMINTERNAL(op) == 0) ? function_type::TRUE_BUILTIN : function_type::BUILTIN;
        else  /*the weird case of NewBuiltin2 , where op is a language expression*/
            fn_type = function_type::TRUE_BUILTIN;
        print_entry_info(call, op, rho, fn_type);

        UNPROTECT(3);

        gc_toggle_restore(gc_enabled);
    }

    static void specialsxp_entry(const SEXP call, const SEXP op, const SEXP rho) {
        int gc_enabled = gc_toggle_off();

        PROTECT(call);
        PROTECT(op);
        PROTECT(rho);

        print_entry_info(call, op, rho, function_type::SPECIAL);

        UNPROTECT(3);

        gc_toggle_restore(gc_enabled);
    }

    static void print_exit_info(const SEXP call, const SEXP op, const SEXP rho, function_type fn_type) {
        int gc_enabled = gc_toggle_off();

        PROTECT(call);
        PROTECT(op);
        PROTECT(rho);

        builtin_info_t info = rec.builtin_exit_get_info(call, op, rho, fn_type);
        rec.builtin_exit_process(info);

        STATE(fun_stack).pop_back();
        STATE(curr_env_stack).pop();

        UNPROTECT(3);

        gc_toggle_restore(gc_enabled);
    }

    static void builtin_exit(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
        int gc_enabled = gc_toggle_off();

        PROTECT(call);
        PROTECT(op);
        PROTECT(rho);
        PROTECT(retval);

        function_type fn_type;
        if(TYPEOF(op)==BUILTINSXP)
            fn_type = (PRIMINTERNAL(op) == 0) ? function_type::TRUE_BUILTIN : function_type::BUILTIN;
        else
            fn_type = function_type::TRUE_BUILTIN;
        print_exit_info(call, op, rho, fn_type);

        UNPROTECT(4);

        gc_toggle_restore(gc_enabled);
    }

    static void specialsxp_exit(const SEXP call, const SEXP op, const SEXP rho, const SEXP retval) {
        int gc_enabled = gc_toggle_off();

        PROTECT(call);
        PROTECT(op);
        PROTECT(rho);
        PROTECT(retval);

        print_exit_info(call, op, rho, function_type::SPECIAL);

        UNPROTECT(4);

        gc_toggle_restore(gc_enabled);
    }

    static void promise_created(const SEXP prom, const SEXP rho) {
        int gc_enabled = gc_toggle_off();

        PROTECT(prom);
        PROTECT(rho);

        prom_basic_info_t info = rec.create_promise_get_info(prom, rho);
        rec.promise_created_process(info);
        if(info.prom_id >= 0) { // maybe we don't need this check
            rec.promise_lifecycle_process({info.prom_id, 0, STATE(gc_trigger_counter)});
        }
        UNPROTECT(2);

        gc_toggle_restore(gc_enabled);
    }

    // Promise is being used inside a function body for the first time.
    static void force_promise_entry(const SEXP symbol, const SEXP rho) {
        int gc_enabled = gc_toggle_off();

        PROTECT(symbol);
        PROTECT(rho);

        prom_info_t info = rec.force_promise_entry_get_info(symbol, rho);
        rec.force_promise_entry_process(info);
        if(info.prom_id >= 0) {
          rec.promise_lifecycle_process({info.prom_id, 1, STATE(gc_trigger_counter)});
        }

        UNPROTECT(2);

        gc_toggle_restore(gc_enabled);
    }

    static void force_promise_exit(const SEXP symbol, const SEXP rho, const SEXP val) {
        int gc_enabled = gc_toggle_off();

        PROTECT(symbol);
        PROTECT(rho);
        PROTECT(val);

        prom_info_t info = rec.force_promise_exit_get_info(symbol, rho, val);
        rec.force_promise_exit_process(info);

        UNPROTECT(3);

        gc_toggle_restore(gc_enabled);
    }

    static void promise_lookup(const SEXP symbol, const SEXP rho, const SEXP val) {
        int gc_enabled = gc_toggle_off();

        PROTECT(symbol);
        PROTECT(rho);
        PROTECT(val);

        prom_info_t info = rec.promise_lookup_get_info(symbol, rho, val);
        if (info.prom_id >= 0) {
            rec.promise_lookup_process(info);
            rec.promise_lifecycle_process({info.prom_id, 1, STATE(gc_trigger_counter)});
        }

        UNPROTECT(3);

        gc_toggle_restore(gc_enabled);
    }

    static void gc_promise_unmarked(const SEXP promise) {
        int gc_enabled = gc_toggle_off();

        PROTECT(promise);
        prom_addr_t addr = get_sexp_address(promise);
        prom_id_t id = get_promise_id(promise);
        auto & promise_origin = STATE(promise_origin);

        if (id >= 0) {
          rec.promise_lifecycle_process({id, 2, STATE(gc_trigger_counter)});
        }

        auto iter = promise_origin.find(id);
        if (iter != promise_origin.end()) {
            // If this is one of our traced promises,
            // delete it from origin map because it is ready to be GCed
            promise_origin.erase(iter);
            //Rprintf("Promise %#x deleted.\n", id);
        }

        unsigned int prom_type = TYPEOF(PRCODE(promise));
        unsigned int orig_type = (prom_type == 21) ? TYPEOF(BODY_EXPR(PRCODE(promise))) : 0;
        prom_key_t key(addr, prom_type, orig_type);
        STATE(promise_ids).erase(key);
        UNPROTECT(1);

        gc_toggle_restore(gc_enabled);
    }

    static void gc_entry(R_size_t size_needed) {
        STATE(gc_trigger_counter) = 1 + STATE(gc_trigger_counter);
    }

    static void gc_exit(int gc_count, double vcells, double ncells) {
        gc_info_t info = rec.gc_exit_get_info(gc_count, vcells, ncells);
        info.counter = STATE(gc_trigger_counter);
        rec.gc_exit_process(info);
    }

    static void vector_alloc(int sexptype, long length, long bytes, const char* srcref) {
        type_gc_info_t info {STATE(gc_trigger_counter), sexptype, length, bytes};
        rec.vector_alloc_process(info);
    }

    static void jump_ctxt(const SEXP rho, const SEXP val) {
        int gc_enabled = gc_toggle_off();

        PROTECT(rho);
        PROTECT(val);

        vector<call_id_t> unwound_calls;
        vector<prom_id_t> unwound_promises;
        unwind_info_t info;

        tracer_state().adjust_stacks(rho, info);

        rec.unwind_process(info);

        UNPROTECT(2);

        gc_toggle_restore(gc_enabled);
    }
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
void register_hooks_with(rdt_handler * h) {
    REG_HOOKS_BEGIN(h, trace_promises<Rec>);
        ADD_HOOK(begin);
        ADD_HOOK(end);
        ADD_HOOK(function_entry);
        ADD_HOOK(function_exit);
        ADD_HOOK(builtin_entry);
        ADD_HOOK(builtin_exit);
        ADD_HOOK(specialsxp_entry);
        ADD_HOOK(specialsxp_exit);
        ADD_HOOK(gc_promise_unmarked);
        ADD_HOOK(force_promise_entry);
        ADD_HOOK(force_promise_exit);
        ADD_HOOK(promise_created);
        ADD_HOOK(promise_lookup);
        ADD_HOOK(vector_alloc);
        ADD_HOOK(gc_entry);
        ADD_HOOK(gc_exit);
        ADD_HOOK(jump_ctxt);
    REG_HOOKS_END;
}

rdt_handler *setup_promises_tracing(SEXP options) {
    tracer_conf_t new_conf = get_config_from_R_options(options);
    tracer_conf.update(new_conf);

    rdt_handler *h = (rdt_handler *) malloc(sizeof(rdt_handler));
    if (tracer_conf.output_format == OutputFormat::TRACE) {
        register_hooks_with<trace_recorder_t>(h);
    }
    else if (tracer_conf.output_format == OutputFormat::SQL) {
        register_hooks_with<sql_recorder_t>(h);
    }
    else if (tracer_conf.output_format == OutputFormat::PREPARED_SQL) {
        register_hooks_with<psql_recorder_t>(h);
    }
    else { // TRACE_AND_SQL
        register_hooks_with<compose<trace_recorder_t, sql_recorder_t>>(h);
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

    return h;
}

// FIXME do we need this function anymore?
void cleanup_promises_tracing(/*rdt_handler *h,*/ SEXP options) {
}
