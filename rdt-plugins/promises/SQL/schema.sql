-- SQLite3 schema 4Þ Rdt promise tracer

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
    call_id integer not null,
    --[ keys ]-----------------------------------------------------------------
    foreign key (call_id) references functions
);

create table if not exists calls (
    --[ identity ]-------------------------------------------------------------
    id integer primary key, -- if CALL_ID is off this is equal to SEXP pointer
    -- pointer integer not null, -- we're not using this at all
    --[ data ]-----------------------------------------------------------------
    function_name text,
    callsite text,
    --[ relations ]------------------------------------------------------------
    function_id integer not null,
    parent_id integer not null, -- ID of call that executed current call
    --[ keys ]-----------------------------------------------------------------
    foreign key (function_id) references functions,
    foreign key (parent_id) references calls
);

create table if not exists promises (
    --[ identity ]-------------------------------------------------------------
    id integer primary key, -- equal to promise pointer SEXP
    type integer null,
    original_type integer null -- if type is BCODE (21) then this is the type before compilation
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
    event_type integer not null, -- 0x0: lookup, 0xf: force
    --[ relations ]------------------------------------------------------------
    promise_id integer not null,
    from_call_id integer not null,
    in_call_id integer not null,
    lifestyle integer not null,
    effective_distance_from_origin integer not null,
    actual_distance_from_origin integer not null,
    --[ keys ]-----------------------------------------------------------------
    foreign key (promise_id) references promises,
    foreign key (from_call_id) references calls,
    foreign key (in_call_id) references calls
);

create table if not exists promise_lifecycle (
    --[ relation ]-------------------------------------------------------------
    promise_id integer not null,
    --[ data ]-----------------------------------------------------------------
    event  integer not null, --- 0x0: creation, -- 0x1: lookup -- 0x2: unmark
    gc_trigger_counter integer not null,
    --[ keys ]-----------------------------------------------------------------
    foreign key (promise_id) references promises,
    foreign key (gc_trigger_counter) references gc_trigger
);

create table if not exists gc_trigger (
    --[ identity ]-------------------------------------------------------------
    counter integer primary key,
    --[ data ]-----------------------------------------------------------------
    ncells real not null,
    vcells real not null
);

create table if not exists type_distribution (
    --[ relation ]-------------------------------------------------------------
    gc_trigger_counter integer not null,
    --[ data ]-----------------------------------------------------------------
    type integer not null,
    length integer not null,
    bytes integer not null,
    --[ keys ]-----------------------------------------------------------------
    foreign key (gc_trigger_counter) references gc_trigger
);

-- Warning: The prepared statement generator assumes semicolons are used only to separate statements in this file,
--          If you use a semicolon for anything else in this file,
--                                                    that prepared statement generator will go explosively crazy.
--
--                                                                                         -love, the Sign Painter
