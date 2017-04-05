#!/bin/bash
rm trace-from-sql.sqlite
cat src/library/rdt/sql/schema.sql trace.sql | sqlite3 trace-from-sql.sqlite
