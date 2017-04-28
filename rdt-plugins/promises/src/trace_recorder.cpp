//
// Created by nohajc on 29.3.17.
//

#include <cstring>
#include <sstream>

#include "trace_recorder.h"
#include "tracer_conf.h"
#include "tracer_state.h"

using namespace std;

enum class TraceLinePrefix {
    ENTER, EXIT, ENTER_AND_EXIT
};

enum class PromiseEvaluationEvent {
    FORCE, LOOKUP
};

inline void prepend_prefix(stringstream &stream, TraceLinePrefix prefix, bool indent, bool as_sql_comment) {
    if (as_sql_comment)
        stream << "-- ";

    if (indent)
        stream << string(STATE(indent), ' ');

    switch (prefix) {
        case TraceLinePrefix::ENTER:
            stream << ">> ";
            break;

        case TraceLinePrefix::EXIT:
            stream << "<< ";
            break;

        case TraceLinePrefix::ENTER_AND_EXIT:
            stream << "<> ";
            break;
    }
}

string function_call_info_line(TraceLinePrefix prefix, const closure_info_t &info, bool indent, bool as_sql_comment,
                               bool call_id_is_pointer) {
    stringstream stream;
    prepend_prefix(stream, prefix, indent, as_sql_comment);

    auto num_fmt = call_id_is_pointer ? hex : dec;
    string num_pref =  call_id_is_pointer ? "0x" : "";

    stream << "call ";
    switch (info.fn_type) {
        case function_type::CLOSURE: // Always this...
            stream << "closure";
            break;
        case function_type::SPECIAL:
            stream << "special";
            break;
        case function_type::BUILTIN:
            stream << "built-in";
    }

    if (info.name.empty())
        stream << " name=<unknown>";
    else
        stream << " name=" << info.name;

    stream << " call_id="  << num_pref << num_fmt << info.call_id
           << " function_id=" << info.fn_id;

    stream << " from_call_id=" << info.parent_call_id;

    if (info.loc.empty())
        stream << " location=<unknown>";
    else
        stream << " location=" << info.loc;

    stream << " compiled=" << (info.fn_compiled ? "true" : "false");

    stream << " arguments={";
    int i = 0;
    for (auto arg_ref : info.arguments.all()) {
        const arg_t & argument = arg_ref.get();
        prom_id_t promise = get<2>(argument);

        stream << get<0>(argument).c_str() << ":" << dec << promise;

        if (i < info.arguments.size() - 1)
            stream << ",";

        ++i;
    }
    stream << "}\n";

    return stream.str();
}

string builtin_or_special_call_info_line(TraceLinePrefix prefix, const builtin_info_t &info, bool indent,
                                         bool as_sql_comment, bool call_id_is_pointer) {
    stringstream stream;
    prepend_prefix(stream, prefix, indent, as_sql_comment);

    auto num_fmt = call_id_is_pointer ? hex : dec;
    string num_pref =  call_id_is_pointer ? "0x" : "";

    stream << "call ";
    switch (info.fn_type) {
        case function_type::SPECIAL:
            stream << "special";
            break;
        case function_type::BUILTIN:
            stream << "built-in";
            break;
        case function_type::CLOSURE: // Just in case
            stream << "closure";
    }

    if (info.name.empty())
        stream << " name=<unknown>";
    else
        stream << " name=" << info.name;

    stream << " call_id="  << num_pref << num_fmt << info.call_id
           << " function_id=" << info.fn_id;

    stream << " from_call_id=" << info.parent_call_id;

    if (info.loc.empty())
        stream << " location=<unknown>";
    else
        stream << " location=" << info.loc;

    stream << " compiled=" << (info.fn_compiled ? "true" : "false");

    stream << "\n";

    return stream.str();
}

string unwind_info_line(TraceLinePrefix prefix, const call_id_t call_id, bool indent, bool as_sql_comment, bool call_id_is_pointer) {
    stringstream stream;
    prepend_prefix(stream, prefix, indent, as_sql_comment);

    auto num_fmt = call_id_is_pointer ? hex : dec;
    string num_pref =  call_id_is_pointer ? "0x" : "";

    stream << "unwind call_id=" << num_pref << num_fmt << call_id << "\n";

    return stream.str();
}

string promise_creation_info_line(TraceLinePrefix prefix, const prom_id_t & prom_id, bool indent, bool as_sql_comment) {
    stringstream stream;
    prepend_prefix(stream, prefix, indent, as_sql_comment);

    stream << "create promise id=" << prom_id << "\n";

    return stream.str();
}

string promise_evaluation_info_line(TraceLinePrefix prefix, PromiseEvaluationEvent event, const prom_info_t & info, bool indent, bool as_sql_comment, bool call_id_is_pointer) {
    stringstream stream;
    prepend_prefix(stream, prefix, indent, as_sql_comment);

    switch (event) {
        case PromiseEvaluationEvent::LOOKUP:
            stream << "lookup promise";
            break;
        case PromiseEvaluationEvent::FORCE:
            stream << "force promise";
            break;
    }

    auto num_fmt = call_id_is_pointer ? hex : dec;
    string num_pref =  call_id_is_pointer ? "0x" : "";

    // FIXME (1) sometimes name is empty and it prints name anyway instead of unknown...
    // FIXME (2) when outputting to file name is always unknown, even though in many cases it should be known
    stream << " name=" << (info.name.empty() ? "<unknown>" : info.name)
           << " id=" << info.prom_id
           << " in_call=" << num_pref << num_fmt << info.in_call_id
           << " from_call=" << num_pref << num_fmt << info.from_call_id << "\n";

    return stream.str();
}

void trace_recorder_t::function_entry(const closure_info_t & info) {
    string statement = function_call_info_line(
            TraceLinePrefix::ENTER,
            info,
            /*indent=*/tracer_conf.pretty_print,
            /*as_sql_comment=*/render_as_sql_comment,
            /*call_id_as_pointer=*/tracer_conf.call_id_use_ptr_fmt);

    multiplexer::output(
            multiplexer::payload_t(statement),
            tracer_conf.outputs);

    if (tracer_conf.pretty_print)
        STATE(indent) += tracer_conf.indent_width;
}

void trace_recorder_t::function_exit(const closure_info_t & info) {
    if (tracer_conf.pretty_print)
        STATE(indent) -= tracer_conf.indent_width;

    string statement = function_call_info_line(
            TraceLinePrefix::EXIT,
            info,
            tracer_conf.pretty_print,
            /*as_sql_comment=*/render_as_sql_comment,
            /*call_id_as_pointer=*/tracer_conf.call_id_use_ptr_fmt);

    multiplexer::output(
            multiplexer::payload_t(statement),
            tracer_conf.outputs);
}

void trace_recorder_t::builtin_entry(const builtin_info_t & info) {
    string statement = builtin_or_special_call_info_line(
            TraceLinePrefix::ENTER,
            info,
            tracer_conf.pretty_print,
            /*as_sql_comment=*/render_as_sql_comment,
            /*call_id_as_pointer=*/tracer_conf.call_id_use_ptr_fmt);

    multiplexer::output(
            multiplexer::payload_t(statement),
            tracer_conf.outputs);

    if (tracer_conf.pretty_print)
        STATE(indent) += tracer_conf.indent_width;
}

void trace_recorder_t::builtin_exit(const builtin_info_t & info) {
    if (tracer_conf.pretty_print)
        STATE(indent) -= tracer_conf.indent_width;

    string statement = builtin_or_special_call_info_line(
            TraceLinePrefix::EXIT,
            info,
            tracer_conf.pretty_print,
            /*as_sql_comment=*/render_as_sql_comment,
            /*call_id_as_pointer=*/tracer_conf.call_id_use_ptr_fmt);

    multiplexer::output(
            multiplexer::payload_t(statement),
            tracer_conf.outputs);
}

void trace_recorder_t::promise_created(const prom_id_t & prom_id) {
    string statement = promise_creation_info_line(
            TraceLinePrefix::ENTER_AND_EXIT,
            prom_id,
            tracer_conf.pretty_print,
            /*as_sql_comment=*/render_as_sql_comment);

    multiplexer::output(
            multiplexer::payload_t(statement),
            tracer_conf.outputs);
}

void trace_recorder_t::force_promise_entry(const prom_info_t & info) {
    string statement = promise_evaluation_info_line(
            TraceLinePrefix::ENTER,
            PromiseEvaluationEvent::FORCE,
            info,
            tracer_conf.pretty_print,
            /*as_sql_comment=*/render_as_sql_comment,
            /*call_id_as_pointer=*/tracer_conf.call_id_use_ptr_fmt);

    multiplexer::output(
            multiplexer::payload_t(statement),
            tracer_conf.outputs);

    if (tracer_conf.pretty_print)
        STATE(indent) += tracer_conf.indent_width;
}

void trace_recorder_t::force_promise_exit(const prom_info_t & info) {
    if (tracer_conf.pretty_print)
        STATE(indent) -= tracer_conf.indent_width;

    string statement = promise_evaluation_info_line(
            TraceLinePrefix::EXIT,
            PromiseEvaluationEvent::FORCE,
            info,
            tracer_conf.pretty_print,
            /*as_sql_comment=*/render_as_sql_comment,
            /*call_id_as_pointer=*/tracer_conf.call_id_use_ptr_fmt);

    multiplexer::output(
            multiplexer::payload_t(statement),
            tracer_conf.outputs);
}

void trace_recorder_t::promise_lookup(const prom_info_t & info) {
    string statement = promise_evaluation_info_line(
            TraceLinePrefix::ENTER_AND_EXIT,
            PromiseEvaluationEvent::LOOKUP,
            info,
            tracer_conf.pretty_print,
            /*as_sql_comment=*/render_as_sql_comment,
            /*call_id_as_pointer=*/tracer_conf.call_id_use_ptr_fmt);

    multiplexer::output(
            multiplexer::payload_t(statement),
            tracer_conf.outputs);
}

void trace_recorder_t::init_recorder() {

}

void trace_recorder_t::start_trace() {
    multiplexer::init(tracer_conf.outputs, tracer_conf.filename, tracer_conf.overwrite);
}

void trace_recorder_t::finish_trace() {
    multiplexer::close(tracer_conf.outputs);
}

void trace_recorder_t::unwind(const vector<call_id_t> & unwound_calls) {
    stringstream statement;

    for (call_id_t call_id : unwound_calls) {
        if (tracer_conf.pretty_print)
            STATE(indent) -= tracer_conf.indent_width;

        statement << unwind_info_line(
                TraceLinePrefix::EXIT,
                call_id,
                tracer_conf.pretty_print,
                /*as_sql_comment=*/render_as_sql_comment,
                /*call_id_as_pointer=*/tracer_conf.call_id_use_ptr_fmt);
    }

    string s = statement.str();
    multiplexer::output(
            multiplexer::payload_t(s),
            tracer_conf.outputs);
}

// TODO rename all `statement`s to something more suitable.
