//
// Created by ksiek on 29.03.17.
//

#include "sql_generator.h"

#include <initializer_list>
#include <string>
#include <fstream>
#include <sstream>
#include <cassert>

using namespace std;

inline sql_stmt_t multirow_insert(string table, int num_columns, int num_rows);

sql_stmt_t sql_generator::begin_transaction() {
    return "begin transaction;\n";
}

sql_stmt_t sql_generator::abort_transaction() {
    return "abort;\n";
}

sql_stmt_t sql_generator::commit_transaction() {
    return "commit;\n";
}

sql_stmt_t sql_generator::insert_function() {
    return "insert into functions values (%s,%s,%s);\n";
}

sql_stmt_t sql_generator::insert_function_call() {
    return "insert into calls values (%s,%s,%s,%s,%s,%s);\n";
}

sql_stmt_t sql_generator::insert_promise() {
    return "insert into promises values (%s);\n";
}

sql_stmt_t sql_generator::insert_promise_evaluation() {
    return "insert into promise_evaluations values (%s,%s,%s,%s);\n";
}

sql_stmt_t sql_generator::insert_promise_associations(int number_of_rows) {
    return multirow_insert("promise_associations", 3, number_of_rows);
}

sql_stmt_t sql_generator::insert_arguments(int number_of_rows) {
    return multirow_insert("arguments", 4, number_of_rows);
}

sql_stmt_t multirow_insert(string table, int num_columns, int num_rows) {
    // Weird cases.
    assert(num_rows > 0 && num_columns > 0);

    // Make a string represeting one row of placeholders/values.
    string value_cell;
    value_cell.reserve(1 + (num_columns - 1) * 3);
    value_cell += "%s";
    for (int i = 1; i < num_columns; i++)
        value_cell += ", %s";

    // Special case for 1 value.
    if (num_rows == 1)
         return "insert into " + table + " values (" + value_cell + ");\n";

    // General case;
    stringstream statement;
    statement << "insert into "
              << table
              << " select "
              << value_cell;

    for (int i = num_rows - 1; i--; num_rows >= 0)
        statement << "union all select "
                  << value_cell;

    statement << ";\n";

    return statement.str();
}

sql_stmt_t sql_generator::create_tables_and_views() {
    ifstream schema_file(RDT_SQL_SCHEMA);
    string schema_string;
    schema_file.seekg(0, ios::end);
    schema_string.reserve(schema_file.tellg());
    schema_file.seekg(0, ios::beg);
    schema_string.assign((istreambuf_iterator<char>(schema_file)), istreambuf_iterator<char>());
    return schema_string.c_str();
}