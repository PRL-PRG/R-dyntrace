#include "multiplexer.h"
#include "tools.h"

#include <fstream>
#include <iostream>
#include <set>

#include "../rdt.h"

using namespace std;

namespace multiplexer {

    ofstream output_file;

#ifdef RDT_SQLITE_SUPPORT
    sqlite3 *sqlite_database;
#endif

    bool init(sink_arr_t outputs, string file_path, bool overwrite) {
        set<Sink> already_initialized;
        bool return_value = true;

        for (auto output_as_wrong_type : outputs) {
            Sink output = (Sink) output_as_wrong_type;

            if (already_initialized.count(output)) {
                cerr << "Warning: trying to initialize the same output type twice (" << output << ")."
                     << "Hillarity may ensue.\n";
                continue;
            } else
                already_initialized.insert(output);

            switch (output) {
                case Sink::PRINT:
                    // Nothing to do.
                    break;

                case Sink::FILE: {
                    output_file.open(file_path, overwrite ? ofstream::trunc : ofstream::app);
                    if (output_file.fail()) {
                        cerr << "Error: could not open file \"" << file_path << "\", "
                             << "message (" << errno << "): " << strerror(errno) << "\n";
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
                        cerr << "Error: could not open DB connection,"
                             << " message (" << outcome << "): "
                             << string(sqlite3_errmsg(sqlite_database)) << "\n";
                        return_value = false;
                    }
#else
                    cerr << "Warning: cannot initialize database connection: no SQLite3 support.\n";
                    return_value = false;
#endif
                    break;
                }
                default:
                    cerr << "Warning: cannot initialize, unknown output sink type: " << output << "\n";
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
                    output_file.close();
                    if (output_file.is_open()) {
                        cerr << "Error: could not close output file, "
                             << "message (" << errno << "): " << strerror(errno) << "\n";
                        return_value = false;
                    }
                    break;
                }

                case Sink::DATABASE: {
#ifdef RDT_SQLITE_SUPPORT
                    sqlite3_close(sqlite_database);
#endif
                    break;
                }
                default:
                    cerr << "Warning: cannot close, unknown output sink type: " << output << "\n";
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
                        cerr << "Warning: cannot print non-text payload (" << tools::enum_cast(payload.type) << "), "
                             << "ignoring.\n";

                    break;

                case Sink::FILE:
                    if (payload.type == Payload::TEXT)
                        output_file << *(payload.text);
#ifdef RDT_SQLITE_SUPPORT
                    else // if (payload.type == Payload::PREPARED_STATEMENT)
                        output_file << sqlite3_sql(payload.prepared_statement);
#endif
                    break;

                case Sink::DATABASE:
#ifdef RDT_SQLITE_SUPPORT
                    if (payload.type == Payload::TEXT) {
                        int outcome = sqlite3_exec(sqlite_database, payload.text->c_str(), NULL, 0, NULL /*&error_msg*/);

                        if (outcome != SQLITE_OK) {
                            cerr << "Error: could not execute SQL query: \"" << *payload.text << "\", "
                                 << "message (" << outcome << "): "
                                 << sqlite3_errmsg(sqlite_database) << "\n";

                            return_value = false;
                        }
                    } else if (payload.type == Payload::PREPARED_STATEMENT) {
                        int outcome = sqlite3_step(payload.prepared_statement);

                        if (outcome != SQLITE_DONE) {
                            cerr << "Error: could not execute prepared statement \""
                                 << sqlite3_sql(payload.prepared_statement) << "\", "
                                 << "message (" << outcome << "): "
                                 << sqlite3_errmsg(sqlite_database) << "\n";

                            return_value = false;
                        }

                        sqlite3_reset(payload.prepared_statement);
                    } else {
                        cerr << "Warning: cannot execute query from unknown payload to DB "
                             << "(" <<  tools::enum_cast(payload.type) << "), ignoring.\n";
                    }
                    break;
#else
                    cerr << "Warning: cannot execute query: no SQLite3 support.\n";

                    return_value = false;
                    break;
#endif

                default:
                    cerr << "Warning: cannot output, unknown output sink type: " << output << "\n";

                    return_value = false;
            }

        return return_value;
    }
}