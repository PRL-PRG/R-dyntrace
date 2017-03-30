//
// Created by ksiek on 29.03.17.
//

#ifndef R_3_3_1_SQL_GENERATOR_H
#define R_3_3_1_SQL_GENERATOR_H

#include <string>
#include <initializer_list>

#define RDT_SQL_SCHEMA "src/library/rdt/sql/schema.sql"

namespace sql_generator {
    typedef std::string sql_stmt_t;
    typedef std::string sql_val_t;

    sql_stmt_t insert_function_statement(sql_val_t id, sql_val_t location, sql_val_t definition);
    sql_stmt_t insert_function_call_statement(sql_val_t id, sql_val_t ptr, sql_val_t name, sql_val_t type, sql_val_t location,  sql_val_t function_id);
    sql_stmt_t insert_arguments_statement(int number_of_arguments);

    sql_stmt_t insert_promise_statement(sql_val_t id);
    sql_stmt_t insert_promise_evaluation_statement(sql_val_t clock, sql_val_t event_type, sql_val_t promise_id, sql_val_t call_id);
    sql_stmt_t insert_promise_associations_statement(int number_of_associations);

    sql_stmt_t begin_transaction_statement();
    sql_stmt_t abort_transaction_statement();
    sql_stmt_t commit_transaction_statement();

    sql_stmt_t create_tables_and_views_statement();
}


#endif //R_3_3_1_SQL_GENERATOR_H
