-- for R/examples/example1.R

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
    id integer primary key -- equal to promise pointer SEXP
);

create table promise_associations (
    --[ relations ]------------------------------------------------------------
    promise_id integer not null,
    call_id integer not null,
    argument_id integer not null,
    --[ keys ]-----------------------------------------------------------------
    foreign key (promise_id) references promises,
    foreign key (call_id) references calls,
    foreign key (argument_id) references arguments
);

create table promise_evaluations (
    --[ data ]-----------------------------------------------------------------
    clock integer primary key autoincrement, -- imposes an order on evaluations
    event_type integer not null, -- 0x0: lookup, 0xf: force, 0x30: peek
    --[ relations ]------------------------------------------------------------
    promise_id integer not null,
    call_id integer not null,
    --[ keys ]-----------------------------------------------------------------
    foreign key (promise_id) references promises,
    foreign key (call_id) references calls
);insert into promise_evaluations values ($next_id,0xf,0x563834276db8,0);
insert into promise_evaluations values ($next_id,0xf,0x563834275fc0,0);
insert into functions values (0x563833096af8,NULL,NULL);
insert into calls values (1,0x5638330c1578,'{',NULL,1,0x563833096af8);
insert into functions values (0x563834265368,'src/library/rdt/R/examples/example1.R:1','function (x)
x');
insert into arguments select 1,'x',0,0x563834265368;
insert into calls values (2,0x563834279000,'f','src/library/rdt/R/examples/example1.R:1',0,0x563834265368);
insert into promises select 0x5638342790a8;
insert into promise_associations select 0x5638342790a8,2,1;
insert into promise_evaluations values ($next_id,0xf,0x5638342790a8,2);
insert into calls values (3,0x563834278ee8,'f','src/library/rdt/R/examples/example1.R:1',0,0x563834265368);
insert into promises select 0x563834278f90;
insert into promise_associations select 0x563834278f90,3,1;
insert into promise_evaluations values ($next_id,0xf,0x563834278f90,3);
insert into calls values (4,0x563834278dd0,'f','src/library/rdt/R/examples/example1.R:1',0,0x563834265368);
insert into promises select 0x563834278e78;
insert into promise_associations select 0x563834278e78,4,1;
insert into promise_evaluations values ($next_id,0xf,0x563834278e78,4);
insert into calls values (5,0x563834278cb8,'f','src/library/rdt/R/examples/example1.R:1',0,0x563834265368);
insert into promises select 0x563834278d60;
insert into promise_associations select 0x563834278d60,5,1;
insert into promise_evaluations values ($next_id,0xf,0x563834278d60,5);
insert into functions values (0x563834267c90,'src/library/rdt/R/examples/example1.R:2','function (x, y, z)
{
    x + y + z
}');
insert into arguments select 2,'x',0,0x563834267c90 union all select 3,'y',1,0x563834267c90 union all select 4,'z',2,0x563834267c90;
insert into calls values (6,0x563834278a18,'g','src/library/rdt/R/examples/example1.R:2',0,0x563834267c90);
insert into promises select 0x563834278c10 union all select 0x563834278ba0 union all select 0x563834278b30;
insert into promise_associations select 0x563834278c10,6,2 union all select 0x563834278ba0,6,3 union all select 0x563834278b30,6,4;
insert into calls values (7,0x563834278a18,'{',NULL,1,0x563833096af8);
insert into functions values (0x5638330998d0,NULL,NULL);
insert into calls values (8,0x563834278a18,'+',NULL,1,0x5638330998d0);
insert into calls values (9,0x563834278a18,'+',NULL,1,0x5638330998d0);
insert into promise_evaluations values ($next_id,0xf,0x563834278c10,6);
insert into calls values (10,0x5638330c1578,'+',NULL,1,0x5638330998d0);
insert into promise_evaluations values ($next_id,0xf,0x563834278ba0,6);
insert into calls values (11,0x5638330c1578,'+',NULL,1,0x5638330998d0);
insert into promise_evaluations values ($next_id,0xf,0x563834278b30,6);
insert into calls values (12,0x5638330c1578,'+',NULL,1,0x5638330998d0);
insert into functions values (0x5638342696c0,'src/library/rdt/R/examples/example1.R:3','function (v, w)
{
    v + 1 + w
}');
insert into arguments select 5,'v',0,0x5638342696c0 union all select 6,'w',1,0x5638342696c0;
insert into calls values (13,0x563834279520,'h','src/library/rdt/R/examples/example1.R:3',0,0x5638342696c0);
insert into promises select 0x563834279670 union all select 0x563834279600;
insert into promise_associations select 0x563834279670,13,5 union all select 0x563834279600,13,6;
insert into calls values (14,0x563834279520,'{',NULL,1,0x563833096af8);
insert into calls values (15,0x563834279520,'+',NULL,1,0x5638330998d0);
insert into calls values (16,0x563834279520,'+',NULL,1,0x5638330998d0);
insert into promise_evaluations values ($next_id,0xf,0x563834279670,13);
insert into promise_evaluations values ($next_id,0xf,0x563834279600,13);
insert into functions values (0x56383426e9f0,'src/library/rdt/R/examples/example1.R:5','function (x, y, z)
x + h(v = z, w = z)');
insert into arguments select 7,'x',0,0x56383426e9f0 union all select 8,'y',1,0x56383426e9f0 union all select 9,'z',2,0x56383426e9f0;
insert into calls values (17,0x5638342791a0,'j','src/library/rdt/R/examples/example1.R:5',0,0x56383426e9f0);
insert into promises select 0x563834279398 union all select 0x563834279328 union all select 0x5638342792b8;
insert into promise_associations select 0x563834279398,17,7 union all select 0x563834279328,17,8 union all select 0x5638342792b8,17,9;
insert into calls values (18,0x5638342791a0,'+',NULL,1,0x5638330998d0);
insert into promise_evaluations values ($next_id,0xf,0x563834279398,17);
insert into calls values (19,0x563834279f10,'h','src/library/rdt/R/examples/example1.R:3',0,0x5638342696c0);
insert into promises select 0x56383427a060 union all select 0x563834279ff0;
insert into promise_associations select 0x56383427a060,19,5 union all select 0x563834279ff0,19,6;
insert into calls values (20,0x563834279f10,'{',NULL,1,0x563833096af8);
insert into calls values (21,0x563834279f10,'+',NULL,1,0x5638330998d0);
insert into calls values (22,0x563834279f10,'+',NULL,1,0x5638330998d0);
insert into promise_evaluations values ($next_id,0xf,0x56383427a060,19);
insert into promise_evaluations values ($next_id,0xf,0x5638342792b8,17);
insert into promise_evaluations values ($next_id,0xf,0x563834279ff0,19);
insert into promise_evaluations values ($next_id,0x0,0x5638342792b8,17);
insert into calls values (23,0x563834279b90,'j','src/library/rdt/R/examples/example1.R:5',0,0x56383426e9f0);
insert into promises select 0x563834279d88 union all select 0x563834279d18 union all select 0x563834279ca8;
insert into promise_associations select 0x563834279d88,23,7 union all select 0x563834279d18,23,8 union all select 0x563834279ca8,23,9;
insert into calls values (24,0x563834279b90,'+',NULL,1,0x5638330998d0);
insert into promise_evaluations values ($next_id,0xf,0x563834279d88,23);
insert into calls values (25,0x563834279998,'h','src/library/rdt/R/examples/example1.R:3',0,0x5638342696c0);
insert into promises select 0x563834279ae8 union all select 0x563834279a78;
insert into promise_associations select 0x563834279ae8,25,5 union all select 0x563834279a78,25,6;
insert into calls values (26,0x563834279998,'{',NULL,1,0x563833096af8);
insert into calls values (27,0x563834279998,'+',NULL,1,0x5638330998d0);
insert into calls values (28,0x563834279998,'+',NULL,1,0x5638330998d0);
insert into promise_evaluations values ($next_id,0xf,0x563834279ae8,25);
insert into promise_evaluations values ($next_id,0xf,0x563834279ca8,23);
insert into promise_evaluations values ($next_id,0xf,0x563834279a78,25);
insert into promise_evaluations values ($next_id,0x0,0x563834279ca8,23);
insert into functions values (0x56383426c058,'src/library/rdt/R/examples/example1.R:4','function (x, y, ...)
{
    h(v = x, w = y) + h(...)
}');
insert into arguments select 10,'x',0,0x56383426c058 union all select 11,'y',1,0x56383426c058 union all select 12,'...[0]',2,0x56383426c058 union all select 13,'...[1]',3,0x56383426c058;
insert into calls values (29,0x56383427a468,'i','src/library/rdt/R/examples/example1.R:4',0,0x56383426c058);
insert into promises select 0x56383427a740 union all select 0x56383427a6d0 union all select 0x56383427a660 union all select 0x56383427a5f0;
insert into promise_associations select 0x56383427a740,29,10 union all select 0x56383427a6d0,29,11 union all select 0x56383427a660,29,12 union all select 0x56383427a5f0,29,13;
insert into calls values (30,0x56383427a468,'{',NULL,1,0x563833096af8);
insert into calls values (31,0x56383427a468,'+',NULL,1,0x5638330998d0);
insert into calls values (32,0x56383427a2a8,'h','src/library/rdt/R/examples/example1.R:3',0,0x5638342696c0);
insert into promises select 0x56383427a3f8 union all select 0x56383427a388;
insert into promise_associations select 0x56383427a3f8,32,5 union all select 0x56383427a388,32,6;
insert into calls values (33,0x56383427a2a8,'{',NULL,1,0x563833096af8);
insert into calls values (34,0x56383427a2a8,'+',NULL,1,0x5638330998d0);
insert into calls values (35,0x56383427a2a8,'+',NULL,1,0x5638330998d0);
insert into promise_evaluations values ($next_id,0xf,0x56383427a3f8,32);
insert into promise_evaluations values ($next_id,0xf,0x56383427a740,29);
insert into promise_evaluations values ($next_id,0xf,0x56383427a388,32);
insert into promise_evaluations values ($next_id,0xf,0x56383427a6d0,29);
insert into calls values (36,0x56383427af38,'h','src/library/rdt/R/examples/example1.R:3',0,0x5638342696c0);
insert into promises select 0x56383427a120 union all select 0x56383427b018;
insert into promise_associations select 0x56383427a120,36,5 union all select 0x56383427b018,36,6;
insert into calls values (37,0x56383427af38,'{',NULL,1,0x563833096af8);
insert into calls values (38,0x56383427af38,'+',NULL,1,0x5638330998d0);
insert into calls values (39,0x56383427af38,'+',NULL,1,0x5638330998d0);
insert into promise_evaluations values ($next_id,0xf,0x56383427a120,36);
insert into promise_evaluations values ($next_id,0xf,0x56383427a660,29);
insert into promise_evaluations values ($next_id,0xf,0x56383427b018,36);
insert into promise_evaluations values ($next_id,0xf,0x56383427a5f0,29);
insert into promise_evaluations values ($next_id,0xf,0x5638331782d8,0);
insert into functions values (0x5638330b8b50,NULL,NULL);
insert into calls values (41,0x5638331055a8,'lazyLoadDBfetch',NULL,1,0x5638330b8b50);
insert into functions values (0x563834272fb8,'src/library/rdt/R/examples/example1.R:7','function (...)
{
    l <- list(...)
    print(l)
}');
insert into arguments select 14,'...[0]',0,0x563834272fb8 union all select 15,'...[1]',1,0x563834272fb8 union all select 16,'...[2]',2,0x563834272fb8;
insert into calls values (40,0x56383427ab48,'l','src/library/rdt/R/examples/example1.R:7',0,0x563834272fb8);
insert into promises select 0x56383427ad78 union all select 0x56383427ad08 union all select 0x56383427ac98;
insert into promise_associations select 0x56383427ad78,40,14 union all select 0x56383427ad08,40,15 union all select 0x56383427ac98,40,16;
insert into calls values (42,0x56383427ab48,'{',NULL,1,0x563833096af8);
insert into functions values (0x563833093f48,NULL,NULL);
insert into calls values (43,0x56383427ab48,'<-',NULL,1,0x563833093f48);
insert into functions values (0x5638330ab4f0,NULL,NULL);
insert into calls values (44,0x56383427ab48,'list',NULL,1,0x5638330ab4f0);
insert into promise_evaluations values ($next_id,0xf,0x56383427ad78,40);
insert into promise_evaluations values ($next_id,0xf,0x56383427ad08,40);
insert into promise_evaluations values ($next_id,0xf,0x56383427ac98,40);
insert into functions values (0x56383427a9f8,NULL,'function (x, ...) 
UseMethod("print")');
insert into arguments select 17,'x',0,0x56383427a9f8;
insert into calls values (45,0x56383427b5a8,'print',NULL,0,0x56383427a9f8);
insert into promises select 0x56383427b688;
insert into promise_associations select 0x56383427b688,45,17;
insert into promise_evaluations values ($next_id,0xf,0x56383427b688,45);
insert into promise_evaluations values ($next_id,0xf,0x563833179900,0);
insert into calls values (46,0x5638331055a8,'lazyLoadDBfetch',NULL,1,0x5638330b8b50);
insert into functions values (0x56383427b378,NULL,'function (x, digits = NULL, quote = TRUE, na.print = NULL, print.gap = NULL, right = FALSE, max = NULL, useSource = TRUE, ...) 
{
    noOpt <- missing(digits) && missing(quote) && missing(na.print) && missing(print.gap) && missing(right) && missing(max) && missing(useSource) && missing(...)
    .Internal(print.default(x, digits, quote, na.print, print.gap, right, max, useSource, noOpt))
}');
insert into arguments select 18,'x',0,0x56383427b378 union all select 19,'digits',1,0x56383427b378 union all select 20,'quote',2,0x56383427b378 union all select 21,'na.print',3,0x56383427b378 union all select 22,'print.gap',4,0x56383427b378 union all select 23,'right',5,0x56383427b378 union all select 24,'max',6,0x56383427b378 union all select 25,'useSource',7,0x56383427b378;
insert into calls values (47,0x56383427cdc8,'print.default',NULL,0,0x56383427b378);
insert into promises select 0x56383427cd90 union all select 0x56383427cd58 union all select 0x56383427cd20 union all select 0x56383427cce8 union all select 0x56383427ccb0 union all select 0x56383427cc78 union all select 0x56383427cc40;
insert into promise_associations select 0x56383427b688,47,18 union all select 0x56383427cd90,47,19 union all select 0x56383427cd58,47,20 union all select 0x56383427cd20,47,21 union all select 0x56383427cce8,47,22 union all select 0x56383427ccb0,47,23 union all select 0x56383427cc78,47,24 union all select 0x56383427cc40,47,25;
insert into promise_evaluations values ($next_id,0x0,0x56383427b688,47);
insert into promise_evaluations values ($next_id,0xf,0x56383427cd90,47);
insert into promise_evaluations values ($next_id,0xf,0x56383427cd58,47);
insert into promise_evaluations values ($next_id,0xf,0x56383427cd20,47);
insert into promise_evaluations values ($next_id,0xf,0x56383427cce8,47);
insert into promise_evaluations values ($next_id,0xf,0x56383427ccb0,47);
insert into promise_evaluations values ($next_id,0xf,0x56383427cc78,47);
insert into promise_evaluations values ($next_id,0xf,0x56383427cc40,47);
insert into functions values (0x5638330adb08,NULL,NULL);
insert into calls values (48,0x56383427cdc8,'print.default',NULL,1,0x5638330adb08);
insert into functions values (0x5638342703e8,'src/library/rdt/R/examples/example1.R:6','function (...)
{
    l <- list(...)
    f(l)
}');
insert into arguments select 26,'...[0]',0,0x5638342703e8 union all select 27,'...[1]',1,0x5638342703e8 union all select 28,'...[2]',2,0x5638342703e8;
insert into calls values (49,0x56383427da28,'k','src/library/rdt/R/examples/example1.R:6',0,0x5638342703e8);
insert into promises select 0x56383427c8f8 union all select 0x56383427c888 union all select 0x56383427c818;
insert into promise_associations select 0x56383427c8f8,49,26 union all select 0x56383427c888,49,27 union all select 0x56383427c818,49,28;
insert into calls values (50,0x56383427da28,'{',NULL,1,0x563833096af8);
insert into calls values (51,0x56383427da28,'<-',NULL,1,0x563833093f48);
insert into calls values (52,0x56383427da28,'list',NULL,1,0x5638330ab4f0);
insert into promise_evaluations values ($next_id,0xf,0x56383427c8f8,49);
insert into promise_evaluations values ($next_id,0xf,0x56383427c888,49);
insert into promise_evaluations values ($next_id,0xf,0x56383427c818,49);
insert into calls values (53,0x56383427d718,'f','src/library/rdt/R/examples/example1.R:1',0,0x563834265368);
insert into promises select 0x56383427d8d8;
insert into promise_associations select 0x56383427d8d8,53,1;
insert into promise_evaluations values ($next_id,0xf,0x56383427d8d8,53);
