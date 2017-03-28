#!/bin/bash
[ -e "$2" ] && rm -vi "$2"
<src/library/rdt/sql/schema.sql sqlite3 "$2"
<src/library/rdt/sql/indices.sql sqlite3 "$2"
<"$1" sqlite3 "$2"

