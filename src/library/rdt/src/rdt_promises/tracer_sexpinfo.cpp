//
// Created by nohajc on 27.3.17.
//

#include "tracer_sexpinfo.h"
#include "tracer_output.h"
#include "tracer_state.h"

#include "../rdt.h"

rid_t get_sexp_address(SEXP e) {
    return (rid_t)e;
}

prom_id_t get_promise_id(SEXP promise) {
    if (promise == R_NilValue)
        return RID_INVALID;
    if (TYPEOF(promise) != PROMSXP)
        return RID_INVALID;

    // A new promise is always created for each argument.
    // Even if the argument is already a promise passed from the caller, it gets re-wrapped.
    prom_addr_t prom_addr = get_sexp_address(promise);
    prom_id_t prom_id;

    auto & promise_ids = STATE(promise_ids);
    auto it = promise_ids.find(prom_addr);
    if (it != promise_ids.end()){
        prom_id = it->second;
    }
    else {
        prom_id = make_promise_id(promise, true);
        // FIXME: prepared SQL statements
        rdt_print(RDT_OUTPUT_SQL, {mk_sql_promise(prom_id)});
    }

    return prom_id;
}

prom_id_t make_promise_id(SEXP promise, bool negative) {
    if (promise == R_NilValue)
        return RID_INVALID;

    prom_addr_t prom_addr = get_sexp_address(promise);
    prom_id_t prom_id;

    if (negative) {
        prom_id = --STATE(prom_neg_id_counter);
    }
    else {
        prom_id = STATE(prom_id_counter)++;
    }
    STATE(promise_ids)[prom_addr] = prom_id;

    return prom_id;
}

fn_addr_t get_function_id(SEXP func) {
    assert(TYPEOF(func) == CLOSXP);
    return get_sexp_address(func);
}

#ifdef RDT_CALL_ID
call_id_t make_funcall_id(SEXP function) {
    if (function == R_NilValue)
        return RID_INVALID;

    return ++STATE(call_id_counter);
}
#else
call_id_t make_funcall_id(SEXP fn_env) {
    assert(fn_env != NULL);
    return get_sexp_address(fn_env);
}
#endif


// Wraper for findVar. Does not look up the value if it already is PROMSXP.
SEXP get_promise(SEXP var, SEXP rho) {
    SEXP prom = R_NilValue;

    if (TYPEOF(var) == PROMSXP) {
        prom = var;
    } else if (TYPEOF(var) == SYMSXP) {
        prom = findVar(var, rho);
    }

    return prom;
}

arg_id_t get_argument_id(fn_addr_t function_id, const string & argument) {
    arg_key_t key = make_pair(function_id, argument);
    auto iterator = STATE(argument_ids).find(key);

    if (iterator != STATE(argument_ids).end()) {
        return iterator->second;
    }

    arg_id_t argument_id = ++STATE(argument_id_sequence);
    STATE(argument_ids)[key] = argument_id;
    return argument_id;
}

arglist_t get_arguments(SEXP op, SEXP rho) {
    arglist_t arguments;

    for (SEXP formals = FORMALS(op); formals != R_NilValue; formals = CDR(formals)) {
        // Retrieve the argument name.
        SEXP argument_expression = TAG(formals);
        SEXP promise_expression = get_promise(argument_expression, rho);

        if (TYPEOF(promise_expression) == DOTSXP) {
            int i = 0;
            for(SEXP dots = promise_expression; dots != R_NilValue; dots = CDR(dots)) {
                SEXP ddd_argument_expression = TAG(dots);
                SEXP ddd_promise_expression = CAR(dots);
                if (ddd_argument_expression == R_NilValue) {
                    arguments.push_back({
                                                get_argument_id(get_function_id(op), to_string(i++)),
                                                get_promise_id(ddd_promise_expression)
                                        }); // ... argument without a name
                }
                else {
                    string ddd_arg_name = get_name(ddd_argument_expression);
                    arguments.push_back({
                                                ddd_arg_name,
                                                get_argument_id(get_function_id(op), ddd_arg_name),
                                                get_promise_id(ddd_promise_expression)
                                        }, true); // this flag says we're inserting a ... argument
                }
            }
        }
        else {
            // Retrieve the promise for the argument.
            // The call SEXP only contains AST to find the actual argument value, we need to search the environment.
            string arg_name = get_name(argument_expression);
            prom_id_t prom_id = get_promise_id(promise_expression);
            if (prom_id != RID_INVALID)
                arguments.push_back({
                                            arg_name,
                                            get_argument_id(get_function_id(op), arg_name),
                                            prom_id
                                    });
        }

    }

    return arguments;
}
