//
// Created by ksiek on 29.03.17.
//

#include "sql_generator.h"
#include "tracer_sexpinfo.h"

#include <cassert>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

namespace sql_generator {

    inline sql_stmt_t make_multirow_insert_statement(string table, vector<string> &values, bool align);

    sql_stmt_t make_insert_function_statement(sql_val_t id, sql_val_t location, sql_val_t definition, sql_val_t type,
                                              sql_val_t compiled) {
        stringstream statement;

        statement << "insert into functions values ("
                  << id << ","
                  << location << ","
                  << definition << ","
                  << type << ","
                  << compiled
                  << ");\n";

        return statement.str();
    }

    sql_stmt_t make_select_max_promise_id_statement() {
        return "select max(id) from promises;\n";
    }

    sql_stmt_t make_select_min_promise_id_statement() {
        // FIXME currently promises with negative IDs are not properly stored in the DB, so I work around it here.
        return "select min(promise_id) from promise_evaluations where promise_id < 1;\n";
        // FIXME this is how it's supposed to be once the promises are fixed
        // return "select min(id) from promises where id < 1;\n";
    }

    sql_stmt_t make_select_max_promise_evaluation_clock_statement() {
        return "select max(clock) from promise_evaluations;\n";
    }

    sql_stmt_t make_select_max_call_id_statement() {
        return "select max(id) from calls;\n";
    }

    sql_stmt_t make_select_max_function_id_statement() {
        return "select max(id) from functions;\n";
    }

    sql_stmt_t make_select_max_argument_id_statement() {
        return "select max(id) from arguments;\n";
    }

    sql_stmt_t make_select_all_function_ids_and_definitions_statement() {
        return "select definition, id from functions;\n";
    }

    sql_stmt_t make_select_all_argument_ids_names_and_function_allegiances_statement() {
        return "select call_id, name, id from arguments;\n";
    }

    sql_stmt_t make_select_all_function_ids_statement() {
        return "select id from functions;\n";
    }
    sql_stmt_t make_select_all_negative_promise_ids_statement() {
        return "select id from promises where id < 0;\n";
    }

    sql_stmt_t make_insert_function_call_statement(sql_val_t id,
                                                   sql_val_t name,
                                                   sql_val_t callsite,
                                                   sql_val_t compiled,
                                                   sql_val_t function_id,
                                                   sql_val_t parent_call_id,
                                                   sql_val_t in_prom_id,
                                                   sql_val_t stack_parent_type,
                                                   sql_val_t stack_parent_id) {

        stringstream statement;

        statement << "insert into calls values ("
                  << id << ","
                  << name << ","
                  << callsite << ","
                  << compiled << ","
                  << function_id << ","
                  << parent_call_id << ","
                  << in_prom_id << ","
                  << stack_parent_type << ","
                  << stack_parent_id
                  << ");\n";

        return statement.str();
    }

    sql_stmt_t make_insert_promise_statement(sql_val_t id,
                                             sql_val_t type,
                                             sql_val_t full_type,
                                             sql_val_t in_prom_id,
                                             sql_val_t stack_parent_type,
                                             sql_val_t stack_parent_id,
                                             sql_val_t promise_stack_depth) {

        stringstream statement;

        statement << "insert into promises values ("
                  << id << ","
                  << type << ","
                  << full_type << ","
                  << in_prom_id << ","
                  << stack_parent_type << ","
                  << stack_parent_id << ","
                  << promise_stack_depth
                  << ");\n";

        return statement.str();
    }

    sql_stmt_t make_insert_promise_evaluation_statement(sql_val_t clock,
                                                        sql_val_t event_type,
                                                        sql_val_t promise_id,
                                                        sql_val_t from_call_id,
                                                        sql_val_t in_call_id,
                                                        sql_val_t in_prom_id,
                                                        sql_val_t lifestyle,
                                                        sql_val_t effective_distance,
                                                        sql_val_t actual_distance,
                                                        sql_val_t stack_parent_type,
                                                        sql_val_t stack_parent_id,
                                                        sql_val_t promise_stack_depth) {

        stringstream statement;

        statement << "insert into promise_evaluations values ("
                  << clock << ","
                  << event_type << ","
                  << promise_id << ","
                  << from_call_id << ","
                  << in_call_id << ","
                  << in_prom_id << ","
                  << lifestyle << ","
                  << effective_distance << ","
                  << actual_distance << ","
                  << stack_parent_type << ","
                  << stack_parent_id << ","
                  << promise_stack_depth
                  << ");\n";

        return statement.str();
    }

    sql_stmt_t make_insert_promise_return_statement(sql_val_t return_type, sql_val_t promise_id, sql_val_t clock) {
        stringstream statement;

        statement << "insert into promise_returns values ("
                  << return_type << ","
                  << promise_id << ","
                  << clock
                  << ");\n";

        return statement.str();
    }

    sql_stmt_t make_insert_promise_lifecycle_statement(sql_val_t promise_id,
                                                       sql_val_t event,
                                                       sql_val_t gc_trigger_counter) {

        stringstream statement;

        statement << "insert into promise_lifecycle values ("
                  << promise_id << ","
                  << event << ","
                  << gc_trigger_counter
                  << ");\n";

        return statement.str();
    }

    sql_stmt_t make_insert_gc_trigger_statement(sql_val_t counter,
                                                sql_val_t ncells,
                                                sql_val_t vcells) {

        stringstream statement;

        statement << "insert into gc_trigger values ("
                  << counter << ","
                  << ncells << ","
                  << vcells
                  << ");\n";

        return statement.str();
    }

    sql_stmt_t make_insert_type_distribution_statement(sql_val_t gc_trigger_counter,
                                                       sql_val_t type,
                                                       sql_val_t length,
                                                       sql_val_t bytes) {
        stringstream statement;

        statement << "insert into type_distribution values ("
                  << gc_trigger_counter << ","
                  << type << ","
                  << length << ","
                  << bytes
                  << ");\n";

        return statement.str();
    }

    sql_stmt_t make_insert_promise_associations_statement(vector<sql_val_cell_t> &associations, bool align) {
        return make_multirow_insert_statement("promise_associations", associations, align);
    }

    sql_stmt_t make_insert_arguments_statement(vector<sql_val_cell_t> &arguments, bool align) {
        return make_multirow_insert_statement("arguments", arguments, align);
    }

    sql_stmt_t make_multirow_insert_statement(string table, vector<string> &values, bool align) {
        int num_rows = values.size();

        // Weird cases.
        assert(num_rows > 0);

        // Special case for 1 value.
        if (num_rows == 1)
            return "insert into " + table + " values (" + values[0] + ");\n";

        // General case;
        stringstream statement;
        auto iter = values.begin();

        statement << "insert into "
                  << table
                  << " select "
                  << *iter;

        for (iter++; iter != values.end(); iter++) {
            if (align)
                statement << "\n" << string(table.size() + 2, ' ');

            statement << " union all select "
                      << *iter;
        }

        statement << ";\n";

        return statement.str();
    }

    sql_stmt_t make_begin_transaction_statement() {
        return "begin transaction;\n";
    }

    sql_stmt_t make_abort_transaction_statement() {
        return "rollback;\n";
    }

    sql_stmt_t make_commit_transaction_statement() {
        return "commit;\n";
    }

    sql_stmt_t make_pragma_statement(sql_val_t option, sql_val_t value) {
        return "pragma " + option + " = " + value + ";\n";
    }

    sql_stmt_t make_insert_matadata_statement(sql_val_t key, sql_val_t value) {
        return "insert into metadata values (" + key + "," + value + ");\n";
    }

    sql_stmt_t make_create_metadata_statement() {
        return "create table if not exists metadata (\n"
                "    --[ data ]-----------------------------------------------------------------\n"
                "    key text not null,\n"
                "    value text\n"
                ");\n";
    }

    sql_stmt_t make_create_functions_statement() {
        return "create table if not exists functions (\n"
                "    --[ identity ]-------------------------------------------------------------\n"
                "    id integer primary key, -- equiv. to pointer of function definition SEXP\n"
                "    --[ data ]-----------------------------------------------------------------\n"
                "    location text,\n"
                "    definition text,\n"
                "    type integer not null, -- 0: closure, 1: built-in, 2: special, 3: primitive aka 'true builtin'\n"
                "                           -- values defined by function_type\n"
                "    compiled boolean not null\n"
                ");\n";
    }

    sql_stmt_t make_create_calls_statement() {
        return  "create table if not exists calls (\n"
                "    --[ identity ]-------------------------------------------------------------\n"
                "    id integer primary key, -- if CALL_ID is off this is equal to SEXP pointer\n"
                "    -- pointer integer not null, -- we're not using this at all\n"
                "    --[ data ]-----------------------------------------------------------------\n"
                "    function_name text,\n"
                "    callsite text,\n"
                "    compiled boolean not null,\n" // TODO remove
                "    --[ relations ]------------------------------------------------------------\n"
                "    function_id integer not null,\n"
                "    parent_id integer not null, -- ID of call that executed current call\n"
                "    in_prom_id integer not null, -- ID of promise in which the call is executed\n"
                "    parent_on_stack_type integer not null, -- promise = 1, call = 2, none = 0\n"
                "    parent_on_stack_id integer null,\n"
                "    --[ keys ]-----------------------------------------------------------------\n"
                "    foreign key (function_id) references functions,\n"
                "    foreign key (parent_id) references calls\n"
                ");\n";
    }

    sql_stmt_t make_create_arguments_statement() {
        return  "create table if not exists arguments (\n"
                "    --[ identity ]-------------------------------------------------------------\n"
                "    id integer primary key, -- arbitrary\n"
                "    --[ data ]-----------------------------------------------------------------\n"
                "    name text not null,\n"
                "    position integer not null,\n"
                "    --[ relations ]------------------------------------------------------------\n"
                "    call_id integer not null,\n"
                "    --[ keys ]-----------------------------------------------------------------\n"
                "    foreign key (call_id) references functions\n"
                ");\n";
    }

    sql_stmt_t make_create_promises_statement() {
        return "create table if not exists promises (\n"
                "    --[ identity ]-------------------------------------------------------------\n"
                "    id integer primary key, -- equal to promise pointer SEXP\n"
                "    type integer not null,\n"
                "    full_type text not null,\n"
                "    in_prom_id integer not null, -- ID of promise in which the promise is executed\n"
                "    parent_on_stack_type integer not null, -- promise = 1, call = 2, none = 0\n"
                "    parent_on_stack_id integer null,\n"
                "    promise_stack_depth integer not null\n"
                ");\n";
    }

   sql_stmt_t make_create_promise_associations_statement() {
       return "create table if not exists promise_associations (\n"
               "    --[ relations ]------------------------------------------------------------\n"
               "    promise_id integer not null,\n"
               "    call_id integer not null,\n"
               "    argument_id integer not null,\n"
               "    --[ keys ]-----------------------------------------------------------------\n"
               "    foreign key (promise_id) references promises,\n"
               "    foreign key (call_id) references calls,\n"
               "    foreign key (argument_id) references arguments\n"
               ");\n";
   }

    sql_stmt_t make_create_promise_evaluations_statement() {
        return "create table if not exists promise_evaluations (\n"
               "    --[ data ]-----------------------------------------------------------------\n"
               "    clock integer primary key autoincrement, -- imposes an order on evaluations\n"
               "    event_type integer not null, -- 0x0: lookup, 0xf: force\n"
               "    --[ relations ]------------------------------------------------------------\n"
               "    promise_id integer not null,\n"
               "    from_call_id integer not null,\n"
               "    in_call_id integer not null,\n"
               "    in_prom_id integer not null, -- ID of promise in which the promise is executed\n"
               "    lifestyle integer not null, -- 0: virgin, 1: local, 2: branch-local/grandchild,\n"
               "                                -- 3: escaped/leaked, 4: immediate-local,\n"
               "                                -- 5: immediate-branch-local/child\n"
               "    effective_distance_from_origin integer not null,\n"
               "    actual_distance_from_origin integer not null,\n"
               "    parent_on_stack_type integer not null, -- promise = 1, call = 2, none = 0\n"
               "    parent_on_stack_id integer null,\n"
               "    promise_stack_depth integer not null,\n"
               "    --[ keys ]-----------------------------------------------------------------\n"
               "    foreign key (promise_id) references promises,\n"
               "    foreign key (from_call_id) references calls,\n"
               "    foreign key (in_call_id) references calls\n"
               ");\n";
    }


    // Whenever we evaluate a promise, we add the information about it here. I add two foreign keys for
    // convenience, at the expense of disk space.
    sql_stmt_t make_create_promise_returns_statement() {
        return "create table if not exists promise_returns (\n"
                "--[ data ]-----------------------------------------------------------------\n"
                "type integer not null, \n"
                "--[ relations ]------------------------------------------------------------\n"
                "promise_id integer not null,\n"
                "clock integer not null,\n"
                "--[ keys ]-----------------------------------------------------------------\n"
                "foreign key (promise_id) references promises,\n"
                "foreign key (clock) references promise_evaluations\n"
                ");\n";
    }

    sql_stmt_t make_create_promise_lifecycle_statement() {
        return "create table if not exists promise_lifecycle (\n" // TODO should be plural
               "--[ relation ]--------------------------------------------------------------\n"
                "promise_id integer not null,\n"
                "--[ data ]-----------------------------------------------------------------\n"
                "event_type integer not null,\n" // TODO what are they
                "gc_trigger_counter integer not null,\n"
                "--[ keys ]-----------------------------------------------------------------\n"
                "foreign key (promise_id) references promises,\n"
                "foreign key (gc_trigger_counter) references gc_trigger\n"
                ");\n";
    }

    sql_stmt_t make_create_gc_trigger_statement() {
       return "create table if not exists gc_trigger (\n" // TODO should be plural
              "--[ identity ]-------------------------------------------------------------\n"
              "counter integer primary key,\n"
              "--[ data ]-----------------------------------------------------------------\n"
              "ncells real not null,\n"
              "vcells real not null\n"
              ");\n";
    }

    sql_stmt_t make_create_type_distribution_statement() {
      return "create table if not exists type_distribution (\n" // TODO should be plural
             "--[ relation ]-------------------------------------------------------------\n"
             "gc_trigger_counter integer not null,\n"
             "--[ data ]-----------------------------------------------------------------\n"
             "type integer not null,\n"
             "length integer not null,\n"
             "bytes integer not null,\n"
             "--[ keys ]-----------------------------------------------------------------\n"
             "foreign key (gc_trigger_counter) references gc_trigger\n"
             ");\n";
    }

    vector<sql_stmt_t> split_into_individual_statements(sql_stmt_t statement_blob) {
        vector<sql_stmt_t> statements;
        stringstream statement;

        for (char c : statement_blob) {
            statement << c;

            if (c == ';') {
                statements.push_back(statement.str());
                statement.str(string()); // clear
            }
        }

        // The remainder is probably garbage...
        // It won't compile on its own since it doesn't end with a semicolon.
        // I'll append it to the most recent statement, so as not to lose information.
        statements.back() += statement.str();
        return statements;
    }

    sql_val_cell_t join(std::initializer_list<sql_val_t> values) {
        stringstream cell;
        bool first = true;
        for (sql_val_t value : values) {
            if (first)
                first = false;
            else
                cell << ",";
            cell << value;
        }
        return cell.str();
    }

    sql_val_t from_int(int i) {
        return to_string(i);
    }

    sql_val_t from_hex(int h) {
        stringstream output;
        output << "0x" << hex << h;
        return output.str();
    }

    sql_val_t from_nullable_string(std::string s) {
        return s.empty() ? "null" : s;
    }

    sql_val_t wrap_nullable_string(std::string s) {
        return s.empty() ? "null" : "'" + s + "'";
    }

    string escape_sql_quote_string(string s) {
        size_t position = 0;
        while ((position = s.find("'", position)) != string::npos) {
            s.replace(position, 1, "''");
            position += 2; // length of "''"
        }
        return s;
    }

    sql_val_t wrap_and_escape_nullable_string(std::string s) {
        return s.empty() ? "null" : "'" + escape_sql_quote_string(s) + "'";
    }

    sql_val_t from_nullable_cstring(const char *s) {
        return s == NULL ? "null" : string(s);
    }

//    sql_val_t next_from_sequence() {
//        return "$next_id";
//    }

    string from_stack_event(stack_event_t event) {
        switch(event.type) {
            case stack_type::CALL:
                return from_int(event.call_id);
            case stack_type::PROMISE:
                return from_int(event.promise_id);
            case stack_type::NONE:
                return "null";
        }
    }
}
