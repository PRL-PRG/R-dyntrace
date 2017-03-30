//
// Created by ksiek on 29.03.17.
//

#ifndef R_3_3_1_SQL_GENERATOR_H
#define R_3_3_1_SQL_GENERATOR_H

#include <string>
#include <vector>
//#include <stdarg.h>
#include <initializer_list>

#define RDT_SQL_SCHEMA "src/library/rdt/sql/schema.sql"

namespace sql_generator {
    typedef std::string sql_stmt_t;
    typedef std::string sql_val_t;
    typedef std::string sql_val_cell_t;

    sql_stmt_t make_insert_function_statement(sql_val_t id, sql_val_t location, sql_val_t definition);
    sql_stmt_t make_insert_function_call_statement(sql_val_t id, sql_val_t ptr, sql_val_t name, sql_val_t type, sql_val_t location,  sql_val_t function_id);
    sql_stmt_t make_insert_arguments_statement(std::vector<sql_val_cell_t> arguments);

    sql_stmt_t make_insert_promise_statement(sql_val_t id);
    sql_stmt_t make_insert_promise_evaluation_statement(sql_val_t clock, sql_val_t event_type, sql_val_t promise_id, sql_val_t call_id);
    sql_stmt_t make_insert_promise_associations_statement(std::vector<sql_val_cell_t> associations);

    sql_stmt_t make_begin_transaction_statement();
    sql_stmt_t make_abort_transaction_statement();
    sql_stmt_t make_commit_transaction_statement();

    sql_stmt_t make_create_tables_and_views_statement();

    sql_val_cell_t join(std::initializer_list<sql_val_t>);

    sql_val_t from_int(int i);
    sql_val_t from_hex(int h);
    sql_val_t from_nullable_string(std::string s);
    sql_val_t from_nullable_cstring(const char * s);
}

#endif //R_3_3_1_SQL_GENERATOR_H
