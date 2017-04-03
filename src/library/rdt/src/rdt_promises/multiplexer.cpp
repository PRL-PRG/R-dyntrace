//
// Created by ksiek on 30.03.17.
//


#include "multiplexer.h"

//#include <string>
#include <fstream>

#include "../rdt.h"

FILE *rdt_mux_output_file = NULL;

using namespace std;

namespace multiplexer {
#ifdef RDT_SQLITE_SUPPORT
    sqlite3 *sqlite_database;
#endif

    static bool file_exists(const string &);

    bool init(Sink output, string file_path, bool overwrite) {
        switch (output) {
            case Sink::PRINT:
                return true;

            case Sink::FILE:
                rdt_mux_output_file = fopen(file_path.c_str(), overwrite ? "w" : "a");
                if (!rdt_mux_output_file) {
                    error("Error: could not open file \"%s\", message (%i): %s\n", file_path, errno, strerror(errno));
                    return NULL;
                }
                return true;

            case Sink::DATABASE: {
#ifdef RDT_SQLITE_SUPPORT
                // Remove old DB if necessary.
                if (overwrite && file_exists(file_path))
                    remove(file_path.c_str());

                // Open DB connection.
                int outcome = sqlite3_open(file_path.c_str(), &sqlite_database);

                // FIXME load schema somehow... although i guess in a separate call

                if (outcome != SQLITE_OK) {
                    fprintf(stderr, "Error: could not open DB connection, message (%i) %s\n",
                            outcome,
                            sqlite3_errmsg(sqlite_database));

                    return false;
                }

                return true;

#else
                fprintf(stderr, "Warning: cannot initialize database connection: no SQLite3 support.\n");
                return false;
#endif
            }
            default:
                fprintf(stderr, "Unknown output sink type: %s\n", output);
        }
    }

    // FIXME finalize: close Db, close file etc.

    bool output(payload_t & payload, sink_arr_t outputs) {
        bool ok = true;

        for (auto output : outputs)
            switch (output) {
                case Sink::PRINT:
                    if (payload.type == Payload::TEXT)
                        Rprintf(payload.text->c_str());
                    else
                        fprintf(stderr, "Warning: cannot print non-text payload (%i), ignoring.\n", payload.type);
                    break;

                case Sink::FILE:
                    if (payload.type == Payload::TEXT)
                        fprintf(rdt_mux_output_file, "%s", payload.text->c_str());
                    else
                        fprintf(stderr, "Warning: cannot print non-text payload to file (%i), ignoring.\n", payload.type);
                    break;

                case Sink::DATABASE:
#ifdef RDT_SQLITE_SUPPORT
                    if (payload.type == Payload::TEXT) {
                        int outcome = sqlite3_exec(sqlite_database, payload.text->c_str(), NULL, 0, NULL /*&error_msg*/);

                        if (outcome != SQLITE_OK) {
                            fprintf(stderr, "Error: could not execute SQL query: \"%s\", message (%i): %s\n",
                                    payload.text->c_str(),
                                    outcome,
                                    sqlite3_errmsg(sqlite_database));

                            ok = false;
                        }
                    } else if (payload.type == Payload::PREPARED_STATEMENT) {
                        int outcome = sqlite3_step(payload.prepared_statement);

                        if (outcome != SQLITE_DONE) {
                            fprintf(stderr, "Error: could not execute prepared statement \"%s\", message (%i): %s\n",
                                    sqlite3_sql(payload.prepared_statement),
                                    outcome,
                                    sqlite3_errmsg(sqlite_database));

                            ok = false;
                        }

                        sqlite3_reset(payload.prepared_statement);
                    } else {
                        fprintf(stderr, "Warning: cannot execute query from unknown payload to DB (%i), ignoring.\n",
                                payload.type);
                    }
                    break;
#else
                    fprintf(stderr, "Warning: cannot execute query: no SQLite3 support.\n");

                    ok = false;
                    break;
#endif

                default:
                    fprintf(stderr, "Unknown output sink type: %s\n", output);

                    ok = false;
            }

        return ok;
    }


    static bool file_exists(const string & fname) {
        ifstream f(fname);
        return f.good();
    }


}