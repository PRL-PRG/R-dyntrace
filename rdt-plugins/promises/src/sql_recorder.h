//
// Created by nohajc on 29.3.17.
//

#ifndef R_3_3_1_PSQL_RECORDER_H
#define R_3_3_1_PSQL_RECORDER_H

#include "recorder.h"

class sql_recorder_t : public recorder_t<sql_recorder_t> {
public:
    void init_recorder() {}
    void start_trace(const metadata_t & info);
    void finish_trace(const metadata_t & info);
    void function_entry(const closure_info_t & info);
    void function_exit(const closure_info_t & info) {}
    void builtin_entry(const builtin_info_t & info);
    void builtin_exit(const builtin_info_t & info) {};
    void force_promise_entry(const prom_info_t & info);
    void force_promise_exit(const prom_info_t & info);
    void vector_alloc(const type_gc_info_t & info);
    void gc_exit(const gc_info_t & info);
    void promise_lifecycle(const prom_gc_info_t & info);
    void promise_created(const prom_basic_info_t & info);
    void promise_lookup(const prom_info_t & info);
    void promise_expression_lookup(const prom_info_t & info);
    void unwind(const unwind_info_t &) {};
};

#endif //R_3_3_1_SQL_RECORDER_H
