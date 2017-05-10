-- SQLite3 schema for Rdt

-- 1. Tables containing basic information ferived from trace.
create table if not exists functions (
    --[ identity ]-------------------------------------------------------------
    id integer primary key, -- equiv. to pointer of function definition SEXP
    --[ data ]-----------------------------------------------------------------
    location text,
    definition text,
    type integer not null, -- 0: closure, 1: built-in, 2: special
                                -- values defined by function_type
    compiled boolean not null
);

create table if not exists arguments (
    --[ identity ]-------------------------------------------------------------
    id integer primary key, -- arbitrary
    --[ data ]-----------------------------------------------------------------
    name text not null,
    position integer not null,
    --[ relations ]------------------------------------------------------------
    function_id integer not null,
    --[ keys ]-----------------------------------------------------------------
    foreign key (function_id) references functions
);

create table if not exists calls (
    --[ identity ]-------------------------------------------------------------
    id integer primary key, -- if CALL_ID is off this is equal to SEXP pointer
    pointer integer not null,
    --[ data ]-----------------------------------------------------------------
    function_name text,
    location text,
    --[ relations ]------------------------------------------------------------
    function_id integer not null,
    parent_id integer not null, -- ID of call that executed current call
    --[ keys ]-----------------------------------------------------------------
    foreign key (function_id) references functions,
    foreign key (parent_id) references calls
);

create table if not exists promises (
    --[ identity ]-------------------------------------------------------------
    id integer primary key -- equal to promise pointer SEXP
);

create table if not exists promise_associations (
    --[ relations ]------------------------------------------------------------
    promise_id integer not null,
    call_id integer not null,
    argument_id integer not null,
    --[ keys ]-----------------------------------------------------------------
    foreign key (promise_id) references promises,
    foreign key (call_id) references calls,
    foreign key (argument_id) references arguments
);

create table if not exists promise_evaluations (
    --[ data ]-----------------------------------------------------------------
    clock integer primary key autoincrement, -- imposes an order on evaluations
    event_type integer not null, -- 0x0: lookup, 0xf: force, 0x30: peek
    --[ relations ]------------------------------------------------------------
    promise_id integer not null,
    from_call_id integer not null,
    in_call_id integer not null,
    --[ keys ]-----------------------------------------------------------------
    foreign key (promise_id) references promises,
    foreign key (from_call_id) references calls,
    foreign key (in_call_id) references calls
);

-- 2. Views that agglomerate parts of the information from the trace.
create view if not exists function_evals as
select
    functions.id as function_id,
	count(*) as evaluations
from functions
left outer join calls on functions.id = calls.function_id
group by functions.id;

create view if not exists function_names as
select
    function_id,
	group_concat(distinct_name, ", ") as names
from(
    select distinct
	    functions.id as function_id,
		case
		    when calls.function_name is null then "<undefined>"
		    when calls.function_name = "" then "<undefined>"
		    else calls.function_name
		end as distinct_name
	from functions
	left outer join calls on functions.id = calls.function_id
)
group by function_id;

create view if not exists argument_evals as
select
    arguments.function_id,
    arguments.name,
    arguments.position,
    sum(case when promise_evaluations.event_type = 15 then 1 else 0 end) as forces,
    sum(case when promise_evaluations.event_type = 0 then 1 else 0 end) as lookups,
    sum(case when promise_evaluations.event_type = 15 and promise_evaluations.from_call_id = promise_evaluations.in_call_id then 1 else 0 end) as local_forces,
    sum(case when promise_evaluations.event_type = 0 and promise_evaluations.from_call_id = promise_evaluations.in_call_id  then 1 else 0 end) as local_lookups
from arguments
join promise_associations on arguments.id = promise_associations.argument_id
join promise_evaluations on promise_associations.promise_id = promise_evaluations.promise_id
group by arguments.id;

create view if not exists promise_evaluation_events as
select
	promise_evaluations.from_call_id,
	arguments.name,
	promise_evaluations.promise_id,
	promise_evaluations.event_type,
	promise_evaluations.clock,
	arguments.name ||
	case
		when promise_evaluations.event_type = 0 then '=' --lookup
		when promise_evaluations.event_type = 15 then '!' -- force
		else '?' -- wildcard
	end as event
	--group_concat(arguments.name || "@" || promise_evaluations.clock ||
	--case
	--	when promise_evaluations.event_type = 0 then '=' --lookup
	--	when promise_evaluations.event_type = 15 then '!' -- force
	--	else '?' -- wildcard
	--end, " < ") as annotated_events
from promises
join promise_evaluations on promise_associations.promise_id = promise_evaluations.promise_id
join promise_associations on promise_associations.promise_id = promises.id
join arguments on promise_associations.argument_id = arguments.id;



create view if not exists function_arguments as
select
	functions.id as function_id,
	group_concat(arguments.name, ", ") as arguments
	--group_concat(arguments.name || ":" || arguments.position, ", ") as annotated_arguments
from functions
join arguments as arguments on arguments.function_id = functions.id
group by functions.id
order by functions.id, arguments.position;

-- 3. Output (as views)
-- 3.1 Call graph
create view if not exists out_call_graph as
select
    parent.function_id as caller_function,
    current.function_id as callee_function,
    count(*) as edges
from calls as current
join calls as parent on current.parent_id = parent.id
group by parent.function_id, current.function_id;

-- 3.2 Function strictness
create view if not exists out_strictness as
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

-- 3.3 Function force order FIXME
create view if not exists out_force_order as
select
	calls.function_id,
	calls.id as call_id,
	calls.function_name as alias,
	--function_arguments.arguments as arguments,
	promise_evaluation_events.event as event,
	promise_evaluation_events.promise_id,
	promise_evaluation_events.clock as clock
from calls
join promise_evaluation_events on calls.id = promise_evaluation_events.from_call_id
--join function_arguments on calls.function_id = function_arguments.function_id
order by calls.function_id, clock;

-- Warning: The prepared statement generator assumes semicolons are used only to separate statements in this file,
--          If you use a semicolon for anything else in this file,
--                                                    that prepared statement generator will go explosively crazy.
--
--                                                                                         -love, the Sign Painter
