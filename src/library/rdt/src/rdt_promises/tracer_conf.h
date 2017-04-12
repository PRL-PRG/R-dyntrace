//
// Created by nohajc on 27.3.17.
//

#ifndef R_3_3_1_TRACER_CONF_H
#define R_3_3_1_TRACER_CONF_H

#include <string>

#include "../rdt.h"
#include "multiplexer.h"

using namespace std;

enum class OutputFormat {TRACE, SQL, PREPARED_SQL, TRACE_AND_SQL};

template<typename T>
class option {
    bool is_supplied;
    T value;

public:
    option(const T& val) {
        value = val;
        is_supplied = false;
    }

    option& operator=(const T& val) {
        value = val;
        is_supplied = true;
        return *this;
    }

    bool operator==(const option& opt) const {
        return value == opt.value;
    }

    bool operator!=(const option& opt) const {
        return !operator==(opt);
    }

    bool operator==(const T& val) const {
        return value == val;
    }

    bool operator!=(const T& val) const {
        return !operator==(val);
    }

    T& operator*() {
        return value;
    }

    T* operator->() {
        return &value;
    }

    // Implicit cast to T
    operator T() const {
        return value;
    }

    bool supplied() const {
        return is_supplied;
    }
};

struct tracer_conf_t {
    option<string> filename;
    //option<OutputDestination> output_type; TODO rem
    option<OutputFormat> output_format;
    option<bool> pretty_print;
    option<bool> include_configuration;
    option<int> indent_width;
    option<bool> call_id_use_ptr_fmt;

    option<multiplexer::sink_arr_t> outputs;

    bool overwrite;
    bool reload_state;

    tracer_conf_t();

    template<typename T>
    static inline bool opt_changed(const T& old_opt, const T& new_opt) {
        return new_opt.supplied() && old_opt != new_opt;
    }

    // Update configuration in a smart way
    // (e.g. ignore changes of output type/format if overwrite == false)
    void update(const tracer_conf_t & conf);
};

extern tracer_conf_t tracer_conf;

tracer_conf_t get_config_from_R_options(SEXP options);

#endif //R_3_3_1_TRACER_CONF_H
