#ifndef R_3_3_1_OUTPUT_MULTIPLEXER_H
#define R_3_3_1_OUTPUT_MULTIPLEXER_H

//#ifdef SQLITE3_H
#define RDT_SQLITE_SUPPORT
//#endif

#include <string>
#include <fstream>
//#include <iostream>

#ifdef RDT_SQLITE_SUPPORT
#include <sqlite3.h>
#endif


namespace multiplexer {

    // Output destinations: global variables.

//    extern std::ofstream output_file;

#ifdef RDT_SQLITE_SUPPORT
    extern sqlite3 *sqlite_database;
#endif

    // Structures and helper functions for wrappping data.
    enum Sink {PRINT = 'p', FILE = 'f', DATABASE = 'd'};
    enum class Payload {TEXT, PREPARED_STATEMENT};

    struct payload_t {
        Payload type;
        union {
            std::string * text;
#ifdef RDT_SQLITE_SUPPORT
            sqlite3_stmt * prepared_statement;
#endif
        };

        payload_t(std::string & s) {
            text = new std::string(s);
            type = Payload::TEXT;
        }

        payload_t(sqlite3_stmt * s) {
            prepared_statement = s;
            type = Payload::PREPARED_STATEMENT;
        }

        payload_t(payload_t && payload) {
            type = payload.type;
            if (payload.type == Payload::TEXT)
                text = payload.text;

#ifdef RDT_SQLITE_SUPPORT
            else
                //if (payload.type == Payload::PREPARED_STATEMENT)
                prepared_statement = payload.prepared_statement;
#endif
        }

        payload_t(payload_t & payload) {
            type = payload.type;
            if (payload.type == Payload::TEXT)
                text = new std::string(*payload.text);

#ifdef RDT_SQLITE_SUPPORT
            else
            //if (payload.type == Payload::PREPARED_STATEMENT)
                prepared_statement = payload.prepared_statement;
#endif
        }
        ~payload_t() {
            if (type == Payload::TEXT)
                delete text;
        }
    };

    typedef std::string sink_arr_t;

    // Functions for configuring outputs.
    bool init(sink_arr_t output, std::string file_path, bool overwrite);
    bool close(sink_arr_t output);

    // Function for actually outputting stuff.
    bool output(payload_t && payload, sink_arr_t outputs);
}

#endif //R_3_3_1_OUTPUT_MULTIPLEXER_H
