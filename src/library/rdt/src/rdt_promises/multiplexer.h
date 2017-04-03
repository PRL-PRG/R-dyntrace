//
// Created by ksiek on 30.03.17.
//

#ifndef R_3_3_1_OUTPUT_MULTIPLEXER_H
#define R_3_3_1_OUTPUT_MULTIPLEXER_H

//#ifdef SQLITE3_H
#define RDT_SQLITE_SUPPORT
//#endif

#include <string>

#ifdef RDT_SQLITE_SUPPORT
#include <sqlite3.h>
#endif

//#include <cstdlib>
//#include <cstdio>

// Output destinations: global variables.
extern FILE *rdt_mux_output_file; // FIXME I can't put this stupid thing into a namespace? Fuck off. Can't we use a C++ file type? The plan is to only use it in C++ anyway...

namespace multiplexer {

#ifdef RDT_SQLITE_SUPPORT
    extern sqlite3 *sqlite_database;
#endif

    // Structures and helper functions for wrappping data.
    enum Sink {PRINT = 'p', FILE = 'f', DATABASE = 'd'};
    enum class Payload {TEXT, PREPARED_STATEMENT};

    typedef struct {
        Payload type;
        union {
            std::string * text;
#ifdef RDT_SQLITE_SUPPORT
            sqlite3_stmt * prepared_statement;
#endif
        };
    } payload_t;

    typedef std::string sink_arr_t;

    payload_t & text(std::string *);

#ifdef RDT_SQLITE_SUPPORT
    payload_t & prepared_sql(sqlite3_stmt *);
#endif

    // Functions for configuring outputs and outputting stuff.
    bool init(Sink output, std::string file_path, bool overwrite);
    bool output(payload_t payload, sink_arr_t outputs);
}

#endif //R_3_3_1_OUTPUT_MULTIPLEXER_H
