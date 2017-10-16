#ifndef __SQL_SERIALIZER__
#define __SQL_SERIALIZER__

#include "State.hpp"
#include "sqlite3.h"
#include "utilities.hpp"
#include <stdio.h>
#include <string>

class SqlSerializer {
  public:
    SqlSerializer(const std::string &database_path,
                  const std::string &schema_path, bool verbose = false);
    ~SqlSerializer();
    void serialize_start_trace(const metadata_t &info);
    void serialize_finish_trace(const metadata_t &info);
    void serialize_function_entry(const closure_info_t &info);
    void serialize_function_exit(const closure_info_t &info) {}
    void serialize_builtin_entry(const builtin_info_t &info);
    void serialize_builtin_exit(const builtin_info_t &info) {}
    void serialize_force_promise_entry(const prom_info_t &info, int clock_id);
    void serialize_force_promise_exit(const prom_info_t &info, int clock_id);
    void serialize_promise_created(const prom_basic_info_t &info);
    void serialize_promise_lookup(const prom_info_t &info, int clock_id);
    void serialize_promise_expression_lookup(const prom_info_t &info,
                                             int clock_id);
    void serialize_promise_lifecycle(const prom_gc_info_t &info);
    void serialize_vector_alloc(const type_gc_info_t &info);
    void serialize_gc_exit(const gc_info_t &info);
    void serialize_unwind(const unwind_info_t &info) {}

  private:
    sqlite3_stmt *compile(const char *statement);
    void execute(sqlite3_stmt *statement);
    void open_database(const std::string database_path);
    void close_database();
    void create_tables(const std::string schema_path);
    void prepare_statements();
    void finalize_statements();

    sqlite3_stmt *populate_promise_evaluation_statement(const prom_info_t &info,
                                                        const int type,
                                                        int clock_id);

    sqlite3_stmt *populate_call_statement(const call_info_t &info);

    sqlite3_stmt *
    populate_insert_promise_statement(const prom_basic_info_t &info);

    sqlite3_stmt *
    populate_promise_association_statement(const closure_info_t &info,
                                           int index);

    sqlite3_stmt *populate_metadata_statement(const string key,
                                              const string value);
    sqlite3_stmt *populate_function_statement(const call_info_t &info);

    sqlite3_stmt *populate_insert_argument_statement(const closure_info_t &info,
                                                     int index);
    bool verbose;
    sqlite3 *database = nullptr;
    sqlite3_stmt *insert_metadata_statement = nullptr;
    sqlite3_stmt *insert_function_statement = nullptr;
    sqlite3_stmt *insert_argument_statement = nullptr;
    sqlite3_stmt *insert_call_statement = nullptr;
    sqlite3_stmt *insert_promise_statement = nullptr;
    sqlite3_stmt *insert_promise_association_statement = nullptr;
    sqlite3_stmt *insert_promise_evaluation_statement = nullptr;
    sqlite3_stmt *insert_promise_return_statement = nullptr;
    sqlite3_stmt *insert_promise_lifecycle_statement = nullptr;
    sqlite3_stmt *insert_gc_trigger_statement = nullptr;
    sqlite3_stmt *insert_type_distribution_statement = nullptr;
};

#endif /* __SQL_SERIALIZER__ */
