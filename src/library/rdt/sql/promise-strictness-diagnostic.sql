-- strictness (diagnostic)
-- run:
--     # prepare DB
--     rm example.sqlite
--     cat src/library/rdt/sql/{schema.sql,example-trace.sql} | sqlite3 example.sqlite
--
--     # run query
--     <src/library/rdt/sql/promise-strictness-diagnostic.sql sqlite3 example.sqlite
--
.width 16, 16, 34, 34
.mode column
.headers on
select
    functions.id as "function id",
	calls.function_name as "function variable",
	group_concat(name, ",")  as "arguments",
	group_concat(forced, ",")  as "forced"
from
	functions
	join (
		select
			functions.id as fid,
			arguments.name as name,
			case
				when event_type = 0xF then arguments.name
				else null
			end as forced
		from
			functions
			-- left outer -- use this to get zero-argument functions
			join arguments on functions.id = arguments.function_id
			-- left outer -- use this to get zero-argument functions
			join promises on promises.argument_id = arguments.id
            -- left outer -- use this to get zero-argument functions
            join (
				select promise_id, event_type from promise_evaluations where promise_evaluations.event_type = 0xF
			) on promise_id = promises.id
		order by arguments.position
	) on functions.id = fid
	left outer join calls on calls.function_id = functions.id
group by calls.id;