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

-- TODO http://stackoverflow.com/questions/2359205/copying-data-from-one-sqlite-database-to-another
-- create table out_strictness as
select
	function_names.function_id,
	function_names.names as aliases,
	argument_evals.name as argument,
	function_evals.evaluations,
	argument_evals.forces,
	argument_evals.lookups,
	argument_evals.local_forces,
	argument_evals.local_lookups
from function_names
join function_evals on function_names.function_id = function_evals.function_id
join argument_evals on function_names.function_id = argument_evals.function_id
order by names;