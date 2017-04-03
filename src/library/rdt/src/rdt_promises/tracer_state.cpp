//
// Created by nohajc on 27.3.17.
//

#include "tracer_state.h"
#include "tracer_conf.h"
//#include "tracer_output.h"
#include "tracer_sexpinfo.h"

void tracer_state_t::start_pass(const SEXP prom) {
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
    //rdt_print(OutputFormat::RDT_OUTPUT_SQL, {mk_sql_promise(prom_id)}); // FIXME RE-WRITE WITH NEW FUNCTIONS!!!!
    // FIXME or move somewhere... probablky move somewhere
}

void tracer_state_t::finish_pass() {
    fun_stack.pop();
#ifdef RDT_CALL_ID
    curr_env_stack.pop();
#endif

    promise_origin.clear();
}

void tracer_state_t::adjust_fun_stack(SEXP rho) {
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

        //rdt_print(OutputFormat::RDT_OUTPUT_TRACE, {print_unwind("<=", call_id)}); // FIXME USE NEW FUNCTION API
    }
}

tracer_state_t::tracer_state_t() {
    indent = 0;
    clock_id = 0;
    call_id_counter = 0;
    prom_id_counter = 0;
    prom_neg_id_counter = 0;
    argument_id_sequence = 0;
}

void tracer_state_t::reset() {
    clock_id = 0;
    call_id_counter = 0;
    prom_id_counter = 0;
    prom_neg_id_counter = 0;
    argument_id_sequence = 0;
    already_inserted_functions.clear();
    argument_ids.clear();
    promise_ids.clear();
}

tracer_state_t& tracer_state_t::get_instance() {
    static tracer_state_t tracer_state;
    return tracer_state;
}

