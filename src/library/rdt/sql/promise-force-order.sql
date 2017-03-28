-- order of promise evaluation (forces and lookups) per call
-- run:
--     # prepare DB
--     rm example.sqlite
--     cat src/library/rdt/sql/{schema.sql,indices.sql,examples/trace1.sql} | sqlite3 example.sqlite
--
--     # run query
--     <src/library/rdt/sql/promise-sforce-order.sql sqlite3 example.sqlite
--
.width 16, 16, 32, 32, 64
.mode column
.headers on

select
	calls.function_id,
	calls.id as call_id,
	calls.function_name as alias,
	function_arguments.arguments,
	promise_evaluations_order.events as evaluation_order
from calls
join promise_evaluations_order on calls.id = promise_evaluations_order.call_id
join function_arguments on calls.function_id = function_arguments.function_id;