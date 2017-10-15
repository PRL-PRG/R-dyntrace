#include "tracer.hpp"

static tracer_state_t *create_tracer_state(SEXP options) {
    return new tracer_state_t(
        sexp_to_string(get_named_list_element(options, "database-path")),
        sexp_to_string(get_named_list_element(options, "schema-path")),
        sexp_to_bool(get_named_list_element(options, "verbose"), false));
}

static SqlSerializer *create_tracer_serializer(const tracer_state_t &state) {
    return new SqlSerializer(state.get_database_filepath(),
                             state.get_schema_filepath(),
                             state.get_verbosity_state());
}

static rdt_handler *create_rdt_handler() {

    rdt_handler *h = (rdt_handler *)malloc(sizeof(rdt_handler));

    h->probe_begin = begin;
    h->probe_end = end;
    h->probe_function_entry = function_entry;
    h->probe_function_exit = function_exit;
    h->probe_builtin_entry = builtin_entry;
    h->probe_builtin_exit = builtin_exit;
    h->probe_specialsxp_entry = specialsxp_entry;
    h->probe_specialsxp_exit = specialsxp_exit;
    h->probe_gc_promise_unmarked = gc_promise_unmarked;
    h->probe_force_promise_entry = force_promise_entry;
    h->probe_force_promise_exit = force_promise_exit;
    h->probe_promise_created = promise_created;
    h->probe_promise_lookup = promise_lookup;
    h->probe_promise_expression_lookup = promise_expression_lookup;
    h->probe_vector_alloc = vector_alloc;
    h->probe_gc_entry = gc_entry;
    h->probe_gc_exit = gc_exit;
    h->probe_jump_ctxt = jump_ctxt;

    return h;
}

rdt_handler *setup_promises_tracing(SEXP options) {
    set_tracer_state(create_tracer_state(options));
    set_tracer_serializer(create_tracer_serializer(tracer_state()));
    return create_rdt_handler();
}

void cleanup_promise_tracing(SEXP options) {
    delete &tracer_serializer();
    delete &tracer_state();
}
