#include "multiplexer.h"
#include "tools.h"

//#include <string>
#include <fstream>
#include <set>

#include "../rdt.h"

FILE *rdt_mux_output_file = NULL; // FIXME use fstream

using namespace std;

namespace multiplexer {
#ifdef RDT_SQLITE_SUPPORT
    sqlite3 *sqlite_database;
#endif

    bool init(sink_arr_t outputs, string file_path, bool overwrite) {
        set<Sink> already_initialized;
        bool return_value = true;

        for (auto output_as_wrong_type : outputs) {
            Sink output = (Sink) output_as_wrong_type;

            if (already_initialized.count(output)) {
                fprintf(stderr, "Warning: trying to initialize the same output type twice (%c). Hillarity may ensue.\n",
                        output);
                continue;
            } else
                already_initialized.insert(output);

            switch (output) {
                case Sink::PRINT:
                    // Nothing to do.
                    break;

                case Sink::FILE: {
                    rdt_mux_output_file = fopen(file_path.c_str(), overwrite ? "w" : "a");
                    if (!rdt_mux_output_file) {
                        fprintf(stderr, "Error: could not open file \"%s\", message (%i): %s\n",
                                file_path.c_str(), errno, strerror(errno));
                        return_value = false;
                    }
                    break;
                }

                case Sink::DATABASE: {
#ifdef RDT_SQLITE_SUPPORT
                    // Remove old DB if necessary.
                    if (overwrite && tools::file_exists(file_path))
                        remove(file_path.c_str());

                    // Open DB connection.
                    int outcome = sqlite3_open(file_path.c_str(), &sqlite_database);
                    if (outcome != SQLITE_OK) {
                        fprintf(stderr, "Error: could not open DB connection, message (%i): %s\n",
                                outcome,
                                sqlite3_errmsg(sqlite_database));

                        return_value = false;
                    }
#else
                    fprintf(stderr, "Warning: cannot initialize database connection: no SQLite3 support.\n");
                    return_value = false;
#endif
                    break;
                }
                default:
                    fprintf(stderr, "Cannot initialize, unknown output sink type: %c\n", output);
            }
        }

        return return_value;
    }

    bool close(sink_arr_t outputs) {
        set<Sink> already_closed;
        bool return_value = true;

        for (auto output_as_wrong_type : outputs) {
            Sink output = (Sink) output_as_wrong_type;

            if (already_closed.count(output))
                continue;
            else
                already_closed.insert(output);

            switch (output) {
                case Sink::PRINT:
                    // Nothing to do.
                    break;

                case Sink::FILE: {
                    int outcome = fclose(rdt_mux_output_file);
                    if (outcome != 0) {
                        fprintf(stderr, "Error: could not close output file, message (%i): %s\n",
                                errno, strerror(errno));
                        return_value = false;
                    }
                    rdt_mux_output_file = NULL;
                    break;
                }

                case Sink::DATABASE: {
#ifdef RDT_SQLITE_SUPPORT
                    sqlite3_close(sqlite_database);
#endif
                    break;
                }
                default:
                    fprintf(stderr, "Cannot close, unknown output sink type: %c\n", output);
            }
        }

        return return_value;
    }

    bool output(payload_t && payload, sink_arr_t outputs) {
        bool return_value = true;

        for (auto output : outputs)
            switch (output) {
                case Sink::PRINT:
                    if (payload.type == Payload::TEXT)
                        // Write to a format string to avoid problems if there are %s etc. expressions in payload.text.
                        Rprintf("%s", payload.text->c_str());
                    else
                        fprintf(stderr, "Warning: cannot print non-text payload (%i), ignoring.\n",
                                tools::enum_cast(payload.type));

                    break;

                case Sink::FILE:
                    if (payload.type == Payload::TEXT)
                        fprintf(rdt_mux_output_file, "%s", payload.text->c_str());
                    else
                        fprintf(stderr, "Warning: cannot print non-text payload to file (%i), ignoring.\n",
                                tools::enum_cast(payload.type));
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

                            return_value = false;
                        }
                    } else if (payload.type == Payload::PREPARED_STATEMENT) {
                        int outcome = sqlite3_step(payload.prepared_statement);

                        if (outcome != SQLITE_DONE) {
                            fprintf(stderr, "Error: could not execute prepared statement \"%s\", message (%i): %s\n",
                                    sqlite3_sql(payload.prepared_statement),
                                    outcome,
                                    sqlite3_errmsg(sqlite_database));

                            return_value = false;
                        }

                        sqlite3_reset(payload.prepared_statement);
                    } else {
                        fprintf(stderr, "Warning: cannot execute query from unknown payload to DB (%i), ignoring.\n",
                                tools::enum_cast(payload.type));
                    }
                    break;
#else
                    fprintf(stderr, "Warning: cannot execute query: no SQLite3 support.\n");

                    return_value = false;
                    break;
#endif

                default:
                    fprintf(stderr, "Cannot output, unknown output sink type: %c\n", output);

                    return_value = false;
            }

        return return_value;
    }
}