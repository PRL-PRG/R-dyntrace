#ifndef R_3_3_1_OUTPUT_MULTIPLEXER_H
#define R_3_3_1_OUTPUT_MULTIPLEXER_H

//#ifdef SQLITE3_H
#define RDT_SQLITE_SUPPORT
//#endif

#include <map>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include "tools.h"

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

    // Classes for returning results from the input function.
    class int_result {
    public:
        int result;
#ifdef RDT_SQLITE_SUPPORT
        bool load(sqlite3_stmt * statement) {
           result =  sqlite3_column_int(statement, 0);
        }
#endif
    };

    class int_vector_result {
    public:
        std::vector<int> result;
#ifdef RDT_SQLITE_SUPPORT
        bool load(sqlite3_stmt * statement) {
            result.push_back(sqlite3_column_int(statement, 0));
        }
#endif
    };

    class int_set_result {
    public:
        std::unordered_set<int> result;
#ifdef RDT_SQLITE_SUPPORT
        bool load(sqlite3_stmt * statement) {
            result.insert(sqlite3_column_int(statement, 0));
        }
#endif
    };

    class string_to_int_map_result {
    public:
        std::unordered_map<std::string, int> result;
#ifdef RDT_SQLITE_SUPPORT
        bool load(sqlite3_stmt * statement) {
            std::string s = reinterpret_cast<const char *>(sqlite3_column_text(statement, 0));
            int i = sqlite3_column_int(statement, 1);
            result[s] = i;
        }
#endif
    };

    class int_string_to_int_map_result {
    public:
        std::map<std::pair<int, std::string>, int> result;
#ifdef RDT_SQLITE_SUPPORT
        bool load(sqlite3_stmt * statement) {
            int i1 = sqlite3_column_int(statement, 0);
            std::string s = reinterpret_cast<const char *>(sqlite3_column_text(statement, 1));
            int i2 = sqlite3_column_int(statement, 2);
            result[std::pair<int, std::string>(i1, s)] = i2;
        }
#endif
    };

    class ulong_string_to_ulong_map_result {
    public:
        std::map<std::pair<unsigned long int, std::string>, unsigned long int> result;
#ifdef RDT_SQLITE_SUPPORT
        bool load(sqlite3_stmt * statement) {
            unsigned long int i1 = sqlite3_column_int64(statement, 0);
            std::string s = reinterpret_cast<const char *>(sqlite3_column_text(statement, 1));
            unsigned long int i2 = sqlite3_column_int64(statement, 2);
            result[std::pair<unsigned long int, std::string>(i1, s)] = i2;
        }
#endif
    };

    // Function for actually retrieving and outputting stuff.
    bool output(payload_t && payload, sink_arr_t outputs);

    template<typename T>
    bool input(payload_t && payload, sink_arr_t outputs, T & result) {
        for (auto output : outputs)
            switch (output) {
                case Sink::PRINT:
                case Sink::FILE:
                std::cerr << "Warning: cannot get input from output of type " << output << ". ignoring.\n";
                    break;

                case Sink::DATABASE:
#ifdef RDT_SQLITE_SUPPORT
                    sqlite3_stmt *prepared_statement;
                    if (payload.type == Payload::TEXT) {
                        int outcome = sqlite3_prepare_v2(sqlite_database, payload.text->c_str(), -1,
                                                         &prepared_statement, NULL);

                        if (outcome != SQLITE_OK) {
                            std::cerr << "Error: could not prepare ad-hoc statement: \"" << *payload.text << "\", "
                                 << "message (" << outcome << "): "
                                 << sqlite3_errmsg(sqlite_database) << "\n";

                            return false;
                        }

                    } else if (payload.type == Payload::PREPARED_STATEMENT) {
                        prepared_statement = payload.prepared_statement;
                    } else {
                        std::cerr << "Warning: cannot execute query from unknown payload to DB "
                             << "(" << tools::enum_cast(payload.type) << "), ignoring.\n";
                        return false;
                    }

                    /* either case */ {
                int outcome;

                while (true) {
                    outcome = sqlite3_step(prepared_statement);

                    if (outcome == SQLITE_ROW) {
                        result.load(prepared_statement);
                    } else if (outcome == SQLITE_DONE) {
                        sqlite3_reset(prepared_statement);
                        return true;
                    } else {
                        std::cerr << "Error: could not execute query \""
                                  << sqlite3_sql(prepared_statement) << "\", "
                                  << "message (" << outcome << "): "
                                  << sqlite3_errmsg(sqlite_database) << "\n";

                        sqlite3_reset(prepared_statement);
                        return false;
                    }
                }
            }
#else
                cerr << "Warning: cannot execute query: no SQLite3 support.\n";
                    return false;
#endif
            }

        return false;
    }
}

#endif //R_3_3_1_OUTPUT_MULTIPLEXER_H
