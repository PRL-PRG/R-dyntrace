//
// Created by nohajc on 27.3.17.
//

#ifndef R_3_3_1_TRACER_STATE_H
#define R_3_3_1_TRACER_STATE_H

#include <vector>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <tuple>

#include "tracer_sexpinfo.h"

#include "tools.h"
#include <r.h>

using namespace std;

typedef tuple<call_id_t, fn_id_t, function_type> call_stack_elem_t;
typedef tuple<prom_id_t, unsigned int, unsigned int> prom_key_t;

struct prom_id_triple_hash {
public:
    size_t operator () (const prom_key_t &p) const {
        auto h1 = hash<prom_id_t>{}(get<0>(p));
        auto h2 = hash<unsigned int>{}(get<1>(p));
        auto h3 = hash<unsigned int>{}(get<2>(p));
        // super simple≥...
        return (h1 << 16) | (h2 << 8) | h3;
    }
};

struct tracer_state_t {
    stack<int, vector<int>> curr_fn_indent_level;
    int indent;
    int clock_id; // Should be kept across Rdt calls (unless overwrite is true)
    // Function call stack (may be useful)
    // Whenever R makes a function call, we generate a function ID and store that ID on top of the stack
    // so that we know where we are (e.g. when printing function ID at function_exit hook)
    vector<call_stack_elem_t> fun_stack; // Should be reset on each tracer pass
    stack<env_addr_t , vector<env_addr_t>> curr_env_stack; // Should be reset on each tracer pass

    // Map from promise IDs to call IDs
    unordered_map<prom_id_t, call_id_t> promise_origin; // Should be reset on each tracer pass
    unordered_set<prom_id_t> fresh_promises;
    // Map from promise address to promise ID;
    unordered_map<prom_key_t, prom_id_t, prom_id_triple_hash> promise_ids;
    unordered_map<prom_id_t, int> promise_lookup_gc_trigger_counter;

    call_id_t call_id_counter; // IDs assigned should be globally unique but we can reset it after each pass if overwrite is true)
    prom_id_t fn_id_counter; // IDs assigned should be globally unique but we can reset it after each pass if overwrite is true)
    prom_id_t prom_id_counter; // IDs assigned should be globally unique but we can reset it after each pass if overwrite is true)
    prom_id_t prom_neg_id_counter;

    unordered_map<fn_key_t, fn_id_t> function_ids; // Should be kept across Rdt calls (unless overwrite is true)
    unordered_set<fn_id_t> already_inserted_functions; // Should be kept across Rdt calls (unless overwrite is true)
    unordered_set<fn_id_t> already_inserted_negative_promises; // Should be kept across Rdt calls (unless overwrite is true)
    arg_id_t argument_id_sequence; // Should be globally unique (can reset between tracer calls if overwrite is true)
    map<arg_key_t, arg_id_t> argument_ids; // Should be kept across Rdt calls (unless overwrite is true)
    int gc_trigger_counter; // Incremented each time there is a gc_entry

    void start_pass(const SEXP prom);
    void finish_pass();
    // When doing longjump (exception thrown, etc.) this function gets the target environment
    // and unwinds function call stack until that environment is on top. It also fixes indentation.
    void adjust_fun_stack(SEXP rho, vector<call_id_t> & unwound_calls);

    static tracer_state_t& get_instance();

private:
    tracer_state_t();
    void reset();
};

static inline tracer_state_t& tracer_state() {
    return tracer_state_t::get_instance();
}

// Helper macro for accessing state properties
#define STATE(property) tracer_state().property


#endif //R_3_3_1_TRACER_STATE_H
