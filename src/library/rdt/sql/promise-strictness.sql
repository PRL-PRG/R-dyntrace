-- strictness (aggregative)
-- run:
--     # prepare DB
--     rm example.sqlite
--     cat src/library/rdt/sql/{schema.sql,example-trace.sql} | sqlite3 example.sqlite
--
--     # run query
--     <src/library/rdt/sql/promise-strictness.sql sqlite3 example.sqlite
--
.width 16, 16, 16, 16, 16, 32, 64
.mode column
.headers on

drop view if exists function_evals;
create temporary view
	function_evals
as select
		functions.id as function_id,
		count(*) as evaluations
	from functions left outer join calls on functions.id = calls.function_id
	group by functions.id;

drop view if exists function_names;
create temporary view
	function_names
as select
		function_id,
		group_concat(distinct_name, ", ") as names
	from
		(select distinct
			functions.id as function_id,
			case
				when calls.function_name not null then calls.function_name
				else "<undefined>"
			end as distinct_name
		from functions left outer join calls on functions.id = calls.function_id)
	group by function_id;

drop view if exists argument_forces;
create temporary view
	argument_forces
as select
	promises.id as promise_id,
	promises.call_id as call_id,
	(select distinct arguments.name from arguments where arguments.id = promises.argument_id) as argument_name,
	(select calls.function_id from calls where calls.id = promises.call_id) as function_id,
	(select count(*) from promise_evaluations where promise_evaluations.promise_id = promises.id) >= 1 as forced
from promises;

select
	function_names.function_id,
	function_names.names,
	arguments.name as argument,
	(select
	    function_evals.evaluations
	from function_evals
	where function_evals.function_id = function_names.function_id) as evaluations,

	(select
	    sum(forced)
	from argument_forces
	where
	    argument_forces.function_id = function_names.function_id and
	    argument_forces.argument_name = arguments.name) as forced,

    (select
	    functions.location
	from functions
	where functions.id = function_names.function_id) as function_location,

	(select
	    replace(functions.definition, CHAR(10), '\n')
	from functions
	where functions.id = function_names.function_id) as function_definition
from function_names join arguments on function_names.function_id = arguments.function_id
group by function_names.function_id, arguments.name;
