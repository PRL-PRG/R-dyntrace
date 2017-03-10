-- order of promise evaluation (forces and lookups) per call
-- run:
--     # prepare DB
--     rm example.sqlite
--     cat src/library/rdt/sql/{schema.sql,example-trace.sql} | sqlite3 example.sqlite
--
--     # run query
--     <src/library/rdt/sql/promise-sforce-order.sql sqlite3 example.sqlite
--
.width 16, 16, 34, 34
.mode column
.headers on

select
    calls.id as call_id,
	calls.function_id,
	calls.function_name,

	group_concat(
	    (select
		    arguments.name
	    from promises join arguments on promises.argument_id = arguments.id
	    where promises.id = promise_evaluations.promise_id),
	    " < "
	) as promise_evaluation_order,

	(select
	    functions.location
	from functions
	where functions.id = calls.function_id) as function_location

from calls join promise_evaluations on calls.id = promise_evaluations.call_id
group by calls.id
order by clock;