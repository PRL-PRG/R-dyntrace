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
    //clock_id = 0;

    // We have to make sure the stack is not empty
    // when referring to the promise created by call to Rdt.
    // This is just a dummy call and environment.
    fun_stack.push_back(make_tuple((call_id_t) 0, (fn_id_t) 0, function_type::CLOSURE));
    curr_env_stack.push(0);

    prom_addr_t prom_addr = get_sexp_address(prom);
    prom_id_t prom_id = make_promise_id(prom);
    promise_origin[prom_id] = 0;
}

void tracer_state_t::finish_pass() {
    fun_stack.pop_back();
    curr_env_stack.pop();

    promise_origin.clear();
}

void tracer_state_t::adjust_stacks(SEXP rho, unwind_info_t & info) {
    call_id_t call_id;
    env_addr_t call_addr;

    // XXX remnant of RDT_CALL_ID
    //(call_id = fun_stack.top()) && get_sexp_address(rho) != call_id

    while (!fun_stack.empty() && (call_addr = curr_env_stack.top()) && get_sexp_address(rho) != call_addr) {
        auto elem = fun_stack.back();
        call_id = get<0>(elem);
        curr_env_stack.pop();
        fun_stack.pop_back();

        info.unwound_calls.push_back(call_id);

        while(!full_stack.empty()) {
            auto event = full_stack.back();
            full_stack.pop_back();

            if (event.type == stack_type::CALL)
                if (event.call_id == call_id)
                    goto outside_promise_loop;
        }
        outside_promise_loop:;
    }
}

//void tracer_state_t::adjust_prom_stack(SEXP rho, vector<prom_id_t> & unwound_promises) {
//    prom_id_t prom_id;
//    env_addr_t call_addr;
//
//    // XXX remnant of RDT_CALL_ID
//    //(call_id = fun_stack.top()) && get_sexp_address(rho) != call_id
//
//    while (!fun_stack.empty() && (call_addr = curr_env_stack.top()) && get_sexp_address(rho) != call_addr) {
//        prom_stack_elem_t elem = prom_stack.back();
//        prom_id = elem.first;
//        curr_env_stack.pop();
//
//        fun_stack.pop_back();
//
//        unwound_promises.push_back(prom_id);
//    }
//}

tracer_state_t::tracer_state_t() {
    indent = 0;
    clock_id = 0;
    call_id_counter = 0;
    fn_id_counter = 0;
    prom_id_counter = 0;
    prom_neg_id_counter = 0;
    argument_id_sequence = 0;
    gc_trigger_counter = 0;
}

void tracer_state_t::reset() {
    clock_id = 0;
    call_id_counter = 0;
    fn_id_counter = 0;
    prom_id_counter = 0;
    prom_neg_id_counter = 0;
    argument_id_sequence = 0;
    gc_trigger_counter = 0;
    already_inserted_functions.clear();
    already_inserted_negative_promises.clear();
    promise_lookup_gc_trigger_counter.clear();
    function_ids.clear();
    argument_ids.clear();
    promise_ids.clear();
}

tracer_state_t& tracer_state_t::get_instance() {
    static tracer_state_t tracer_state;
    return tracer_state;
}

