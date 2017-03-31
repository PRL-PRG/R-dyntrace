//
// Created by nohajc on 27.3.17.
//

#ifndef R_3_3_1_TRACER_OUTPUT_H
#define R_3_3_1_TRACER_OUTPUT_H

#include <cstdlib>
#include <cstdio>
#include <string>

#include "tracer_sexpinfo.h"

using namespace std;

enum class OutputFormat: char {RDT_OUTPUT_TRACE, RDT_OUTPUT_SQL, RDT_OUTPUT_BOTH, RDT_OUTPUT_COMPILED_SQLITE};
enum class OutputType: char {RDT_R_PRINT, RDT_FILE, RDT_SQLITE, RDT_R_PRINT_AND_SQLITE};

//extern FILE *output;

void rdt_print(OutputFormat string_format, std::initializer_list<string> strings);
void prepend_prefix(stringstream *stream);

string print_unwind(const char *type, call_id_t call_id);
string print_builtin(const char *type, const char *loc, const char *name, fn_addr_t id, call_id_t call_id);
string print_promise(const char *type, const char *loc, const char *name, prom_id_t id, call_id_t in_call_id, call_id_t from_call_id);
string print_function(const char *type, const char *loc, const char *name, fn_addr_t function_id, call_id_t call_id, arglist_t const& arguments);
//string print_function(const string & type, const string & loc, const string & name, fn_addr_t function_id, call_id_t call_id, arglist_t const& arguments);

/*
 * ===========================================================
 *                            S Q L
 * ===========================================================
 */



#ifdef RDT_SQLITE_SUPPORT
#include <sqlite3.h>

extern sqlite3 *sqlite_database; // TODO does not need to be global maybe?
#endif


void rdt_init_sqlite(const string& filename);
void rdt_close_sqlite();
void rdt_configure_sqlite();
void rdt_begin_transaction();
void rdt_commit_transaction();
void rdt_abort_transaction();

void run_prep_sql_function(fn_addr_t function_id, arglist_t const& arguments, const char* location, const char* definition);
void run_prep_sql_function_call(call_id_t call_id, env_addr_t call_ptr, const char *name, const char* location, int call_type, fn_addr_t function_id);
void run_prep_sql_promise(prom_id_t prom_id);
void run_prep_sql_promise_assoc(arglist_t const& arguments, call_id_t call_id);
void run_prep_sql_promise_evaluation(int event_type, prom_id_t promise_id, call_id_t call_id);

string mk_sql_function(fn_addr_t function_id, arglist_t const& arguments, const char* location, const char* definition);
string mk_sql_function_call(call_id_t call_id, env_addr_t call_ptr, const char *name, const char* location, int call_type, fn_addr_t function_id);
string mk_sql_promise(prom_id_t prom_id);
string mk_sql_promise_assoc(arglist_t const& arguments, call_id_t call_id);
string mk_sql_promise_evaluation(int event_type, prom_id_t promise_id, call_id_t call_id);

#endif //R_3_3_1_TRACER_OUTPUT_H
