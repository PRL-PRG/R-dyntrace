//
// Created by nohajc on 27.3.17.
//

#include "tracer_conf.h"
#include "tracer_output.h"

tracer_conf_t::tracer_conf_t() :
// Config defaults
        filename("tracer.db"),
        output_type(RDT_R_PRINT),
        output_format(RDT_OUTPUT_TRACE),
        pretty_print(true),
        overwrite(false),
        indent_width(4),
#ifdef RDT_CALL_ID
        call_id_use_ptr_fmt(false)
#else
        call_id_use_ptr_fmt(true)
#endif
{}

// Update configuration in a smart way
// (e.g. ignore changes of output type/format if overwrite == false)
void tracer_conf_t::update(const tracer_conf_t & conf) {
#define OPT_CHANGED(opt) opt_changed(opt, conf.opt)

    bool conf_changed =
            OPT_CHANGED(filename) ||
            OPT_CHANGED(output_type) ||
            OPT_CHANGED(output_format) ||
            OPT_CHANGED(pretty_print) ||
            OPT_CHANGED(indent_width) ||
            OPT_CHANGED(call_id_use_ptr_fmt);

    if (conf.overwrite || conf_changed) {
        *this = conf; // updates all members
        overwrite = true;
    }
    else {
        overwrite = false;
    }

#undef OPT_CHANGED
}


tracer_conf_t get_config_from_R_options(SEXP options) {
    tracer_conf_t conf;

    const char *filename_option = get_string(get_named_list_element(options, "path"));
    if (filename_option != NULL)
        conf.filename = filename_option;

    const char *output_format_option = get_string(get_named_list_element(options, "format"));
    if (output_format_option != NULL) {
        if (!strcmp(output_format_option, "trace"))
            conf.output_format = RDT_OUTPUT_TRACE;
        else if (!strcmp(output_format_option, "SQL") || !strcmp(output_format_option, "sql"))
            conf.output_format = RDT_OUTPUT_SQL;
        else if (!strcmp(output_format_option, "PSQL") || !strcmp(output_format_option, "psql"))
            conf.output_format = RDT_OUTPUT_COMPILED_SQLITE;
        else if (!strcmp(output_format_option, "both"))
            conf.output_format = RDT_OUTPUT_BOTH;
        else
            error("Unknown format type: \"%s\"\n", output_format_option);
    }

    //Rprintf("output_format_option=%s->%i\n", output_format_option,output_format);

    const char *output_type_option = get_string(get_named_list_element(options, "output"));
    if (output_type_option != NULL) {
        if (!strcmp(output_type_option, "R") || !strcmp(output_type_option, "r"))
            conf.output_type = RDT_R_PRINT;
        else if (!strcmp(output_type_option, "file"))
            conf.output_type = RDT_FILE;
        else if (!strcmp(output_type_option, "DB") || !strcmp(output_type_option, "db"))
            conf.output_type = RDT_SQLITE;
        else if (!strcmp(output_type_option, "R+DB") || !strcmp(output_type_option, "r+db") ||
                 !strcmp(output_type_option, "DB+R") || !strcmp(output_type_option, "db+r"))
            conf.output_type = RDT_R_PRINT_AND_SQLITE;
        else
            error("Unknown format type: \"%s\"\n", output_type_option);
    }

    //Rprintf("output_type_option=%s->%i\n", output_type_option,output_type);

    SEXP pretty_print_option = get_named_list_element(options, "pretty.print");
    if (pretty_print_option != NULL && pretty_print_option != R_NilValue)
        conf.pretty_print = LOGICAL(pretty_print_option)[0] == TRUE;
    //Rprintf("pretty_print_option=%p->%i\n", (pretty_print_option), pretty_print);

    SEXP indent_width_option = get_named_list_element(options, "indent.width");
    if (indent_width_option != NULL && indent_width_option != R_NilValue)
        if (TYPEOF(indent_width_option) == REALSXP)
            conf.indent_width = (int) *REAL(indent_width_option);
    //Rprintf("indent_width_option=%p->%i\n", indent_width_option, indent_width);

    SEXP overwrite_option = get_named_list_element(options, "overwrite");
    if (overwrite_option != NULL && overwrite_option != R_NilValue)
        conf.overwrite = LOGICAL(overwrite_option)[0] == TRUE;
    //Rprintf("overwrite_option=%p->%i\n", (overwrite_option), overwrite);

#ifdef RDT_CALL_ID
    SEXP synthetic_call_id_option = get_named_list_element(options, "synthetic.call.id");
    if (synthetic_call_id_option != NULL && synthetic_call_id_option == R_NilValue)
        conf.call_id_use_ptr_fmt = LOGICAL(synthetic_call_id_option)[0] == FALSE;
    //Rprintf("call_id_use_ptr_fmt=%p->%i\n", (synthetic_call_id_option), call_id_use_ptr_fmt);
#endif

    return conf;
}

tracer_conf_t tracer_conf; // init default configuration