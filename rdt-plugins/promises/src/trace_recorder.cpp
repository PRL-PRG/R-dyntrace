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
    ENTER, EXIT, ENTER_AND_EXIT, METADATA
};

enum class PromiseEvaluationEvent {
    FORCE, LOOKUP, EXPRESSION_LOOKUP
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

        case TraceLinePrefix::METADATA:
            stream << "## ";
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
            break;
        case function_type::TRUE_BUILTIN:
            stream << "true built-in";
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

    if (info.callsite.empty())
        stream << " callsite=<unknown>";
    else
        stream << " callsite=" << info.callsite;

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

    switch (info.parent_on_stack.type) {
        case stack_type::CALL:
            stream << " parent_id=<call>:" << info.parent_on_stack.call_id;
            break;
        case stack_type::PROMISE:
            stream << " parent_id=<promise>:" << info.parent_on_stack.promise_id;
            break;
        case stack_type::NONE:
            stream << " parent_id=<none>";
            break;
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
        case function_type::TRUE_BUILTIN:
            stream << "true built-in";
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
    stream << " in_prom=" << info.in_prom_id;

    if (info.loc.empty())
        stream << " location=<unknown>";
    else
        stream << " location=" << info.loc;

    if (info.callsite.empty())
        stream << " callsite=<unknown>";
    else
        stream << " callsite=" << info.callsite;

    stream << " compiled=" << (info.fn_compiled ? "true" : "false");

    switch (info.parent_on_stack.type) {
        case stack_type::CALL:
            stream << " parent_id=<call>:" << info.parent_on_stack.call_id;
            break;
        case stack_type::PROMISE:
            stream << " parent_id=<promise>:" << info.parent_on_stack.promise_id;
            break;
        case stack_type::NONE:
            stream << " parent_id=<none>";
            break;
    }

    stream << "\n";

    return stream.str();
}

string unwind_call_info_line(TraceLinePrefix prefix, const call_id_t call_id, bool indent, bool as_sql_comment, bool call_id_is_pointer) {
    stringstream stream;
    prepend_prefix(stream, prefix, indent, as_sql_comment);

    stream << "unwind call_id=" << call_id << "\n";

    return stream.str();
}

string unwind_promises_info_line(TraceLinePrefix prefix, const vector<prom_id_t> & unwound_promises, bool indent, bool as_sql_comment, bool call_id_is_pointer) {
    stringstream stream;
    prepend_prefix(stream, prefix, indent, as_sql_comment);

    stream << "unwind promises=";

    bool first = true;
    for (prom_id_t prom_id : unwound_promises) {
        if(!first)
            stream << " ";
        else
            first = false;
        stream << prom_id;
    }

    stream << "\n";

    return stream.str();
}

string gc_info_line(TraceLinePrefix prefix, const gc_info_t & info, bool indent, bool as_sql_comment) {
    stringstream stream;
    prepend_prefix(stream, prefix, indent, as_sql_comment);
    stream << "counter=" << info.counter;
    stream << " ncells=" << info.ncells;
    stream << " vcells=" << info.vcells;
    stream << "\n";
    return stream.str();
}

string vector_info_line(TraceLinePrefix prefix, const type_gc_info_t & info, bool indent, bool as_sql_comment) {
    stringstream stream;
    prepend_prefix(stream, prefix, indent, as_sql_comment);
    stream << "gc_trigger_counter=" << info.gc_trigger_counter;
    stream << " type=" << info.type;
    stream << " length=" << info.length;
    stream << " bytes=" << info.bytes;
    stream << "\n";
    return stream.str();
}

string promise_lifecycle_info_line(TraceLinePrefix prefix, const prom_gc_info_t & info, bool indent, bool as_sql_comment) {
    stringstream stream;
    prepend_prefix(stream, prefix, indent, as_sql_comment);
    stream << "promise_id=" << info.promise_id;
    stream << " event=" << info.event;
    stream << " gc_trigger_counter=" << info.gc_trigger_counter;
    stream << "\n";
    return stream.str();
}

string promise_creation_info_line(TraceLinePrefix prefix, const prom_basic_info_t & info, bool indent, bool as_sql_comment) {
    stringstream stream;
    prepend_prefix(stream, prefix, indent, as_sql_comment);
    stream << "create promise id=" << info.prom_id
           << " in_prom=" << info.in_prom_id
           << " type=" << sexp_type_to_string(info.prom_type)
           << " full_type=" << full_sexp_type_to_string(info.full_type)
           << " depth=" << info.depth;

    switch (info.parent_on_stack.type) {
        case stack_type::CALL:
            stream << " parent_id=<call>:" << info.parent_on_stack.call_id;
            break;
        case stack_type::PROMISE:
            stream << " parent_id=<promise>:" << info.parent_on_stack.promise_id;
            break;
        case stack_type::NONE:
            stream << " parent_id=<none>";
            break;
    }

    stream << "\n";

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

    stream << " name=" << (info.name.empty() ? "<unknown>" : info.name)
           << " id=" << info.prom_id
           << " in_call=" << info.in_call_id
           << " from_call=" << info.from_call_id
           << " in_prom=" << info.in_prom_id;

    switch (info.lifestyle) {
        case lifestyle_type::LOCAL:
            stream << " lifestyle=local";
            break;
        case lifestyle_type::IMMEDIATE_LOCAL:
            stream << " lifestyle=immediate-local";
            break;
        case lifestyle_type::BRANCH_LOCAL:
            stream << " lifestyle=branch-local";
            break;
        case lifestyle_type::IMMEDIATE_BRANCH_LOCAL:
            stream << " lifestyle=immediate-branch-local";
            break;
        case lifestyle_type::ESCAPED:
            stream << " lifestyle=escaped";
            break;
        case lifestyle_type::VIRGIN:
            stream << " lifestyle=virgin";
            break;
    }

    stream << " distance_from_origin=" << info.effective_distance_from_origin
           << "/" << info.actual_distance_from_origin
           << " type=" << sexp_type_to_string(info.prom_type)
           << " return_type=" << sexp_type_to_string(info.return_type)
           << " depth=" << info.depth;

    switch (info.parent_on_stack.type) {
        case stack_type::CALL:
            stream << " parent_id=<call>:" << info.parent_on_stack.call_id;
            break;
        case stack_type::PROMISE:
            stream << " parent_id=<promise>:" << info.parent_on_stack.promise_id;
            break;
        case stack_type::NONE:
            stream << " parent_id=<none>";
            break;
    }

    stream << "\n";

    return stream.str();
}

string start_info_line(TraceLinePrefix prefix, metadata_t metadata, bool indent, bool as_sql_comment) {
    stringstream stream;

    stream << "\n";
    for(const auto & i : metadata) {
        prepend_prefix(stream, prefix, indent, as_sql_comment);
        stream << i.first << "=" << i.second << "\n";
    }
    stream << "\n";

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

    if (tracer_conf.pretty_print) {
        STATE(indent) += tracer_conf.indent_width;
        STATE(curr_fn_indent_level).push(STATE(indent));
    }
}

void trace_recorder_t::function_exit(const closure_info_t & info) {
    if (tracer_conf.pretty_print) {
        STATE(indent) -= tracer_conf.indent_width;
        STATE(curr_fn_indent_level).pop();
    }

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

    if (tracer_conf.pretty_print) {
        STATE(indent) += tracer_conf.indent_width;
        STATE(curr_fn_indent_level).push(STATE(indent));
    }
}

void trace_recorder_t::builtin_exit(const builtin_info_t & info) {
    if (tracer_conf.pretty_print) {
        STATE(indent) -= tracer_conf.indent_width;
        STATE(curr_fn_indent_level).pop();
    }

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

void trace_recorder_t::promise_created(const prom_basic_info_t & info) {
    string statement = promise_creation_info_line(
            TraceLinePrefix::ENTER_AND_EXIT,
            info,
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

void trace_recorder_t::promise_expression_lookup(const prom_info_t & info) {
    string statement = promise_evaluation_info_line(
            TraceLinePrefix::ENTER_AND_EXIT,
            PromiseEvaluationEvent::EXPRESSION_LOOKUP,
            info,
            tracer_conf.pretty_print,
            /*as_sql_comment=*/render_as_sql_comment,
            /*call_id_as_pointer=*/tracer_conf.call_id_use_ptr_fmt);

    multiplexer::output(
            multiplexer::payload_t(statement),
            tracer_conf.outputs);
}

void trace_recorder_t::promise_lifecycle(const prom_gc_info_t & info) {
    string statement = promise_lifecycle_info_line(
            TraceLinePrefix::ENTER_AND_EXIT,
            info,
            tracer_conf.pretty_print,
            /*as_sql_comment=*/render_as_sql_comment);

    multiplexer::output(
            multiplexer::payload_t(statement),
            tracer_conf.outputs);
}

void trace_recorder_t::gc_exit(const gc_info_t & info) {
    string statement = gc_info_line(
            TraceLinePrefix::ENTER_AND_EXIT,
            info,
            tracer_conf.pretty_print,
            /*as_sql_comment=*/render_as_sql_comment);

    multiplexer::output(
            multiplexer::payload_t(statement),
            tracer_conf.outputs);
}

void trace_recorder_t::vector_alloc(const type_gc_info_t & info) {
    string statement = vector_info_line(
            TraceLinePrefix::ENTER_AND_EXIT,
            info,
            tracer_conf.pretty_print,
            /*as_sql_comment=*/render_as_sql_comment);

    multiplexer::output(
            multiplexer::payload_t(statement),
            tracer_conf.outputs);
}

void trace_recorder_t::init_recorder() {

}

void trace_recorder_t::start_trace(const metadata_t & info) {
    multiplexer::init(tracer_conf.outputs, tracer_conf.filename, tracer_conf.overwrite);

    string statement = start_info_line(
            TraceLinePrefix::METADATA ,
            info,
            /*as_sql_comment=*/render_as_sql_comment,
            /*call_id_as_pointer=*/tracer_conf.call_id_use_ptr_fmt);

    multiplexer::output(
            multiplexer::payload_t(statement),
            tracer_conf.outputs);
}

void trace_recorder_t::finish_trace(const metadata_t & info) {
    string statement = start_info_line(
            TraceLinePrefix::METADATA ,
            info,
            /*as_sql_comment=*/render_as_sql_comment,
            /*call_id_as_pointer=*/tracer_conf.call_id_use_ptr_fmt);

    multiplexer::output(
            multiplexer::payload_t(statement),
            tracer_conf.outputs);

    multiplexer::close(tracer_conf.outputs);
}

void trace_recorder_t::unwind(const unwind_info_t & info) {
    stringstream statement;

    for (call_id_t call_id : info.unwound_calls) {
        if (tracer_conf.pretty_print) {
            STATE(indent) = STATE(curr_fn_indent_level).top();
            STATE(curr_fn_indent_level).pop();
            STATE(indent) -= tracer_conf.indent_width;
        }

        statement << unwind_call_info_line(
                TraceLinePrefix::EXIT,
                call_id,
                tracer_conf.pretty_print,
                /*as_sql_comment=*/render_as_sql_comment,
                /*call_id_as_pointer=*/tracer_conf.call_id_use_ptr_fmt);
    }

    if (!info.unwound_promises.empty()) {
        statement << unwind_promises_info_line(
                TraceLinePrefix::EXIT,
                info.unwound_promises,
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
