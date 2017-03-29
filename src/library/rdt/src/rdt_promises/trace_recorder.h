//
// Created by nohajc on 29.3.17.
//

#ifndef R_3_3_1_TRACEPR_H
#define R_3_3_1_TRACEPR_H

#include "recorder.h"

class trace_recorder_t : public recorder_t<trace_recorder_t> {
public:
    void function_entry(const call_info_t & info);
    void function_exit(const call_info_t & info);
    void builtin_entry(const call_info_t & info);
    void builtin_exit(const call_info_t & info);
    void force_promise_entry(const prom_info_t & info);
    void force_promise_exit(const prom_info_t & info);
    void promise_created(const prom_id_t & prom_id);
    void promise_lookup(const prom_info_t & info);
};

#endif //R_3_3_1_TRACEPR_H
