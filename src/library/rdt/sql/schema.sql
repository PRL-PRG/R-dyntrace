-- SQLite3 schema for Rdt
create table functions (
    --[ identity ]-------------------------------------------------------------
    id integer primary key, -- equiv. to pointer of function definition SEXP
    --[ data ]-----------------------------------------------------------------
    location text,
    definition text
);

create table arguments (
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

create table calls (
    --[ identity ]-------------------------------------------------------------
    id integer primary key, -- if CALL_ID is off this is equal to SEXP pointer
    pointer integer not null,
    --[ data ]-----------------------------------------------------------------
    function_name text,
    location text,
    call_type integer not null, -- 0: function, 1: built-in
    --[ relations ]------------------------------------------------------------
    function_id integer not null,
    --[ keys ]-----------------------------------------------------------------
    foreign key (function_id) references functions
);

create table promises (
    --[ identity ]-------------------------------------------------------------
    id integer primary key, -- equal to promise pointer SEXP
    --[ relations ]------------------------------------------------------------
    call_id integer not null,
    argument_id integer not null,
    --[ keys ]-----------------------------------------------------------------
    foreign key (call_id) references calls,
    foreign key (argument_id) references arguments
);

create table promise_evaluations (
    --[ data ]-----------------------------------------------------------------
    clock integer primary key autoincrement, -- imposes an order on evaluations
    event_type integer not null, -- 0x0: lookup, 0xf: force
    --[ relations ]------------------------------------------------------------
    promise_id integer not null,
    call_id integer not null,
    --[ keys ]-----------------------------------------------------------------
    foreign key (promise_id) references promises,
    foreign key (call_id) references calls
);