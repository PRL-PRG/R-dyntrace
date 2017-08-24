//
// Created by nohajc on 29.3.17.
//

#ifndef R_3_3_1_TRACEPR_H
#define R_3_3_1_TRACEPR_H

#include "recorder.h"
#include "sql_recorder.h"

class trace_recorder_t : public recorder_t<trace_recorder_t> {
    bool render_as_sql_comment;
public:
    trace_recorder_t(bool render_as_sql_comment = false) {
        this->render_as_sql_comment = render_as_sql_comment;
    }

    void init_recorder();
    void start_trace(const metadata_t & info);
    void finish_trace(const metadata_t & info);
    void function_entry(const closure_info_t & info);
    void function_exit(const closure_info_t & info);
    void builtin_entry(const builtin_info_t & info);
    void builtin_exit(const builtin_info_t & info);
    void force_promise_entry(const prom_info_t & info);
    void force_promise_exit(const prom_info_t & info);
    void promise_created(const prom_basic_info_t & info);
    void promise_lookup(const prom_info_t & info);
    void promise_expression_lookup(const prom_info_t & info);
    void promise_lifecycle(const prom_gc_info_t & info);
    void vector_alloc(const type_gc_info_t & info);
    void gc_exit(const gc_info_t & info);
    void unwind(const unwind_info_t & info);
};

// Specialize composite tracer for SQL comments

#define CONCAT_IMPL(a, b) a ## b
#define CONCAT(a, b) CONCAT_IMPL(a, b)

// This is the part where we set `render_as_sql_comment` to true
//by calling the constructor with non-default value
#define trace_recorder_t_CONSTR trace_recorder_t(true)
#define sql_recorder_t_CONSTR sql_recorder_t()

#define CONSTRUCT(type) CONCAT(type, _CONSTR)


#define SQL_COMPOSE(type1, type2) \
    template<> \
    class compose<type1, type2> : public compose_impl<type1, type2> { \
    public: \
        std::tuple<type1, type2> rec; \
        compose() : rec(CONSTRUCT(type1), CONSTRUCT(type2)) {} \
    }

SQL_COMPOSE(trace_recorder_t, sql_recorder_t);
SQL_COMPOSE(sql_recorder_t, trace_recorder_t);

#endif //R_3_3_1_TRACEPR_H
