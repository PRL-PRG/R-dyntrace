#include "hooks.hpp"

void begin(const SEXP prom) {
    PROTECT(prom);
    tracer_state().start_pass(prom);

    metadata_t metadata;
    //= rec.get_metadata_from_environment();
    get_environment_metadata(metadata);
    get_current_time_metadata(metadata, "START");

    tracer_serializer().serialize_start_trace(metadata);
    UNPROTECT(1);
}

void end() {
    tracer_state().finish_pass();

    metadata_t metadata;
    get_current_time_metadata(metadata, "END");
    tracer_serializer().serialize_finish_trace(metadata);

    if (!tracer_state().fun_stack.empty()) {
        Rprintf("Function stack is not balanced: %d remaining.\n",
                tracer_state().fun_stack.size());
        tracer_state().fun_stack.clear();
    }

    if (!tracer_state().full_stack.empty()) {
        Rprintf("Function/promise stack is not balanced: %d remaining.\n",
                tracer_state().full_stack.size());
        tracer_state().full_stack.clear();
    }
}

// Triggered when entering function evaluation.
void function_entry(const SEXP call, const SEXP op, const SEXP rho) {
    PROTECT(call);
    PROTECT(op);
    PROTECT(rho);

    closure_info_t info = function_entry_get_info(call, op, rho);

    // Push function ID on function stack
    tracer_state().fun_stack.push_back(
        make_tuple(info.call_id, info.fn_id, info.fn_type));
    tracer_state().curr_env_stack.push(info.call_ptr);

    tracer_serializer().serialize_function_entry(info);

    auto &fresh_promises = tracer_state().fresh_promises;
    // Associate promises with call ID
    for (auto arg_ref : info.arguments.all()) {
        const arg_t &argument = arg_ref.get();
        auto &promise = get<2>(argument);
        auto it = fresh_promises.find(promise);

        if (it != fresh_promises.end()) {
            tracer_state().promise_origin[promise] = info.call_id;
            fresh_promises.erase(it);
        }
    }

    UNPROTECT(3);
}

void function_exit(const SEXP call, const SEXP op, const SEXP rho,
                   const SEXP retval) {
    PROTECT(call);
    PROTECT(op);
    PROTECT(rho);
    PROTECT(retval);

    closure_info_t info = function_exit_get_info(call, op, rho);
    tracer_serializer().serialize_function_exit(info);

    // Current function ID is popped in function_exit_get_info
    tracer_state().curr_env_stack.pop();

    UNPROTECT(4);
}

void print_entry_info(const SEXP call, const SEXP op, const SEXP rho,
                      function_type fn_type) {
    PROTECT(call);
    PROTECT(op);
    PROTECT(rho);

    builtin_info_t info = builtin_entry_get_info(call, op, rho, fn_type);
    tracer_serializer().serialize_builtin_entry(info);

    tracer_state().fun_stack.push_back(
        make_tuple(info.call_id, info.fn_id, info.fn_type));
    tracer_state().curr_env_stack.push(info.call_ptr | 1);

    UNPROTECT(3);
}

void builtin_entry(const SEXP call, const SEXP op, const SEXP rho) {
    PROTECT(call);
    PROTECT(op);
    PROTECT(rho);

    function_type fn_type;
    if (TYPEOF(op) == BUILTINSXP)
        fn_type = (PRIMINTERNAL(op) == 0) ? function_type::TRUE_BUILTIN
                                          : function_type::BUILTIN;
    else /*the weird case of NewBuiltin2 , where op is a language expression*/
        fn_type = function_type::TRUE_BUILTIN;
    print_entry_info(call, op, rho, fn_type);

    UNPROTECT(3);
}

void specialsxp_entry(const SEXP call, const SEXP op, const SEXP rho) {
    PROTECT(call);
    PROTECT(op);
    PROTECT(rho);

    print_entry_info(call, op, rho, function_type::SPECIAL);

    UNPROTECT(3);
}

void print_exit_info(const SEXP call, const SEXP op, const SEXP rho,
                     function_type fn_type) {
    PROTECT(call);
    PROTECT(op);
    PROTECT(rho);

    builtin_info_t info = builtin_exit_get_info(call, op, rho, fn_type);
    tracer_serializer().serialize_builtin_exit(info);

    tracer_state().fun_stack.pop_back();
    tracer_state().curr_env_stack.pop();

    UNPROTECT(3);
}

void builtin_exit(const SEXP call, const SEXP op, const SEXP rho,
                  const SEXP retval) {
    PROTECT(call);
    PROTECT(op);
    PROTECT(rho);
    PROTECT(retval);

    function_type fn_type;
    if (TYPEOF(op) == BUILTINSXP)
        fn_type = (PRIMINTERNAL(op) == 0) ? function_type::TRUE_BUILTIN
                                          : function_type::BUILTIN;
    else
        fn_type = function_type::TRUE_BUILTIN;
    print_exit_info(call, op, rho, fn_type);

    UNPROTECT(4);
}

void specialsxp_exit(const SEXP call, const SEXP op, const SEXP rho,
                     const SEXP retval) {
    PROTECT(call);
    PROTECT(op);
    PROTECT(rho);
    PROTECT(retval);

    print_exit_info(call, op, rho, function_type::SPECIAL);

    UNPROTECT(4);
}

void promise_created(const SEXP prom, const SEXP rho) {
    PROTECT(prom);
    PROTECT(rho);

    prom_basic_info_t info = create_promise_get_info(prom, rho);
    tracer_serializer().serialize_promise_created(info);
    if (info.prom_id >= 0) { // maybe we don't need this check
        tracer_serializer().serialize_promise_lifecycle(
            {info.prom_id, 0, tracer_state().gc_trigger_counter});
    }
    UNPROTECT(2);
}

// Promise is being used inside a function body for the first time.
void force_promise_entry(const SEXP symbol, const SEXP rho) {
    PROTECT(symbol);
    PROTECT(rho);

    prom_info_t info = force_promise_entry_get_info(symbol, rho);
    tracer_serializer().serialize_force_promise_entry(info);
    if (info.prom_id >= 0) {
        tracer_serializer().serialize_promise_lifecycle(
            {info.prom_id, 1, tracer_state().gc_trigger_counter});
    }

    UNPROTECT(2);
}

void force_promise_exit(const SEXP symbol, const SEXP rho, const SEXP val) {
    PROTECT(symbol);
    PROTECT(rho);
    PROTECT(val);

    prom_info_t info = force_promise_exit_get_info(symbol, rho, val);
    tracer_serializer().serialize_force_promise_exit(info);

    UNPROTECT(3);
}

void promise_lookup(const SEXP symbol, const SEXP rho, const SEXP val) {
    PROTECT(symbol);
    PROTECT(rho);
    PROTECT(val);

    prom_info_t info = promise_lookup_get_info(symbol, rho, val);
    if (info.prom_id >= 0) {
        tracer_serializer().serialize_promise_lookup(info);
        tracer_serializer().serialize_promise_lifecycle(
            {info.prom_id, 1, tracer_state().gc_trigger_counter});
    }

    UNPROTECT(3);
}

void promise_expression_lookup(const SEXP prom, const SEXP rho) {
    PROTECT(prom);
    PROTECT(rho);

    prom_info_t info = promise_expression_lookup_get_info(prom, rho);
    if (info.prom_id >= 0) {
        tracer_serializer().serialize_promise_expression_lookup(info);
        tracer_serializer().serialize_promise_lifecycle(
            {info.prom_id, 1, tracer_state().gc_trigger_counter});
    }

    UNPROTECT(2);
}

void gc_promise_unmarked(const SEXP promise) {
    PROTECT(promise);
    prom_addr_t addr = get_sexp_address(promise);
    prom_id_t id = get_promise_id(promise);
    auto &promise_origin = tracer_state().promise_origin;

    if (id >= 0) {
        tracer_serializer().serialize_promise_lifecycle(
            {id, 2, tracer_state().gc_trigger_counter});
    }

    auto iter = promise_origin.find(id);
    if (iter != promise_origin.end()) {
        // If this is one of our traced promises,
        // delete it from origin map because it is ready to be GCed
        promise_origin.erase(iter);
        // Rprintf("Promise %#x deleted.\n", id);
    }

    unsigned int prom_type = TYPEOF(PRCODE(promise));
    unsigned int orig_type =
        (prom_type == 21) ? TYPEOF(BODY_EXPR(PRCODE(promise))) : 0;
    prom_key_t key(addr, prom_type, orig_type);
    tracer_state().promise_ids.erase(key);
    UNPROTECT(1);
}

void gc_entry(R_size_t size_needed) {
    tracer_state().gc_trigger_counter = 1 + tracer_state().gc_trigger_counter;
}

void gc_exit(int gc_count, double vcells, double ncells) {
    gc_info_t info = gc_exit_get_info(gc_count, vcells, ncells);
    info.counter = tracer_state().gc_trigger_counter;
    tracer_serializer().serialize_gc_exit(info);
}

void vector_alloc(int sexptype, long length, long bytes, const char *srcref) {
    type_gc_info_t info{tracer_state().gc_trigger_counter, sexptype, length,
                        bytes};
    tracer_serializer().serialize_vector_alloc(info);
}

void jump_ctxt(const SEXP rho, const SEXP val) {
    PROTECT(rho);
    PROTECT(val);

    vector<call_id_t> unwound_calls;
    vector<prom_id_t> unwound_promises;
    unwind_info_t info;

    tracer_state().adjust_stacks(rho, info);

    tracer_serializer().serialize_unwind(info);

    UNPROTECT(2);
}
