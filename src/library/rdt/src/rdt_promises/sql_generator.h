//
// Created by ksiek on 29.03.17.
//

#ifndef R_3_3_1_SQL_GENERATOR_H
#define R_3_3_1_SQL_GENERATOR_H

#include <string>
#include <initializer_list>

#define RDT_SQL_SCHEMA "src/library/rdt/sql/schema.sql"

typedef std::string sql_stmt_t;

class sql_generator {
    public:
    sql_stmt_t begin_transaction();
    sql_stmt_t abort_transaction();
    sql_stmt_t commit_transaction();

    sql_stmt_t create_tables_and_views();

    sql_stmt_t insert_function();
    sql_stmt_t insert_function_call();
    sql_stmt_t insert_arguments(int number_of_arguments);

    sql_stmt_t insert_promise();
    sql_stmt_t insert_promise_evaluation();
    sql_stmt_t insert_promise_associations(int number_of_associations);
};


#endif //R_3_3_1_SQL_GENERATOR_H
