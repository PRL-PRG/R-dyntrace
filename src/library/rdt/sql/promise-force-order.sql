-- order of promise evaluation (forces and lookups) per call
-- run:
--     # prepare DB
--     rm example.sqlite
--     cat src/library/rdt/sql/{schema.sql,example-trace1.sql} | sqlite3 example.sqlite
--
--     # run query
--     <src/library/rdt/sql/promise-sforce-order.sql sqlite3 example.sqlite
--
.width 16, 16, 16, 32, 32, 16, 64
.mode column
.headers on

select
    calls.id as call_id,
	calls.function_id,
	calls.function_name,

	(select
		group_concat(arguments.name, ",")
		from arguments
		where arguments.function_id = calls.function_id
		group by arguments.function_id
	) as arguments,

	group_concat(
	    (
		select
		    arguments.name -- || ":" || promise_evaluations.clock
			|| case
				when promise_evaluations.event_type = 0 then '=' -- lookup
				when promise_evaluations.event_type = 15 then '!' -- force
				when promise_evaluations.event_type = 48 then  '?' -- peek
			end
	    from promises
		join promise_associations on promise_associations.promise_id = promises.id
		join arguments on promise_associations.argument_id = arguments.id
		--select
		 --   arguments.name -- || ":" || promise_evaluations.clock
	    --from promises join arguments on promises.argument_id = arguments.id
	    where promises.id = promise_evaluations.promise_id),
	    " < "
	) as promise_evaluation_order,

	(select
	    functions.location
	from functions
	where functions.id = calls.function_id) as function_location,

	(select
	    replace(functions.definition, CHAR(10), '\n')
	from functions
	where functions.id = calls.function_id) as function_definition

from calls join promise_evaluations on calls.id = promise_evaluations.call_id
group by calls.id
order by clock;