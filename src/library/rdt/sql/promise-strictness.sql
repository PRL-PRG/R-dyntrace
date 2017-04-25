-- strictness (aggregative)
-- run:
--     # prepare DB
--     rm example.sqlite
--     cat src/library/rdt/sql/{schema.sql,indices.sql,examples/trace1.sql} | sqlite3 example.sqlite
--
--     # run query
--     <src/library/rdt/sql/promise-strictness.sql sqlite3 example.sqlite
--
.width 16, 32, 24, 8, 8, 8
.mode column
.headers on

-- create table strictness as
select
	function_names.function_id,
	function_names.names as aliases,
	argument_evals.name as argument,
	function_evals.evaluations,
	argument_evals.forces,
	argument_evals.lookups
from function_names
join function_evals on function_names.function_id = function_evals.function_id
join argument_evals on function_names.function_id = argument_evals.function_id
order by names;