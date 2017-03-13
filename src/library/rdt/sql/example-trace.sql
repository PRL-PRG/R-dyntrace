-- Install in database;
--     rm example.sqlite
--     cat src/library/rdt/sql/{schema.sql,example-trace.sql} | sqlite3 example.sqlite

-- This represents the following R code:
--     f <- function(x) x
--     g <- function(x,y,z) {x+y+z}
--     h <- function(v,w) {v+1+w}
--     i <- function(x,y,...) {h(v=x,w=y) + h(...)}
--     j <- function(x,y,z) x + h(v=z,w=z)
--     trace.promises.sql({f(1); f(2); f(3); f(4); g(1+1,2+2,3+3); h(1,1); j(1,1,1); j(1,1,1); i(1,1,1,1)})

insert into promise_evaluations values ($next_id,0xf,0x559c2edaf9e0,0);
insert into promise_evaluations values ($next_id,0xf,0x559c2edaebe8,0);
insert into functions values (0x559c2d7ee9a8,NULL,NULL);
insert into calls values (1,0x559c2d819428,'{',NULL,1,0x559c2d7ee9a8);
insert into functions values (0x559c2eda9670,'<console>','function (x) 
x');
insert into arguments select 1,'x',0,0x559c2eda9670;
insert into calls values (2,0x559c2edb0cf8,'f','<console>',0,0x559c2eda9670);
insert into promises select 0x559c2edb0da0,2,1;
insert into promise_evaluations values ($next_id,0xf,0x559c2edb0da0,2);
insert into calls values (3,0x559c2edb0be0,'f','<console>',0,0x559c2eda9670);
insert into promises select 0x559c2edb0c88,3,1;
insert into promise_evaluations values ($next_id,0xf,0x559c2edb0c88,3);
insert into calls values (4,0x559c2edb1a30,'f','<console>',0,0x559c2eda9670);
insert into promises select 0x559c2edb0b70,4,1;
insert into promise_evaluations values ($next_id,0xf,0x559c2edb0b70,4);
insert into calls values (5,0x559c2edb1918,'f','<console>',0,0x559c2eda9670);
insert into promises select 0x559c2edb19c0,5,1;
insert into promise_evaluations values ($next_id,0xf,0x559c2edb19c0,5);
insert into functions values (0x559c2eda9ce0,'<console>','function (x, y, z) 
{
    x + y + z
}');
insert into arguments select 2,'x',0,0x559c2eda9ce0 union all select 3,'y',1,0x559c2eda9ce0 union all select 4,'z',2,0x559c2eda9ce0;
insert into calls values (6,0x559c2edb1678,'g','<console>',0,0x559c2eda9ce0);
insert into promises select 0x559c2edb1870,6,2 union all select 0x559c2edb1800,6,3 union all select 0x559c2edb1790,6,4;
insert into calls values (7,0x559c2edb1678,'{',NULL,1,0x559c2d7ee9a8);
insert into functions values (0x559c2d7f1780,NULL,NULL);
insert into calls values (8,0x559c2edb1678,'+',NULL,1,0x559c2d7f1780);
insert into calls values (9,0x559c2edb1678,'+',NULL,1,0x559c2d7f1780);
insert into promise_evaluations values ($next_id,0xf,0x559c2edb1870,6);
insert into calls values (10,0x559c2d819428,'+',NULL,1,0x559c2d7f1780);
insert into promise_evaluations values ($next_id,0xf,0x559c2edb1800,6);
insert into calls values (11,0x559c2d819428,'+',NULL,1,0x559c2d7f1780);
insert into promise_evaluations values ($next_id,0xf,0x559c2edb1790,6);
insert into calls values (12,0x559c2d819428,'+',NULL,1,0x559c2d7f1780);
insert into functions values (0x559c2edaa388,'<console>','function (v, w) 
{
    v + 1 + w
}');
insert into arguments select 5,'v',0,0x559c2edaa388 union all select 6,'w',1,0x559c2edaa388;
insert into calls values (13,0x559c2edb2180,'h','<console>',0,0x559c2edaa388);
insert into promises select 0x559c2edb1368,13,5 union all select 0x559c2edb12f8,13,6;
insert into calls values (14,0x559c2edb2180,'{',NULL,1,0x559c2d7ee9a8);
insert into calls values (15,0x559c2edb2180,'+',NULL,1,0x559c2d7f1780);
insert into calls values (16,0x559c2edb2180,'+',NULL,1,0x559c2d7f1780);
insert into promise_evaluations values ($next_id,0xf,0x559c2edb1368,13);
insert into promise_evaluations values ($next_id,0xf,0x559c2edb12f8,13);
insert into functions values (0x559c2edab7b8,'<console>','function (x, y, z) 
x + h(v = z, w = z)');
insert into arguments select 7,'x',0,0x559c2edab7b8 union all select 8,'y',1,0x559c2edab7b8 union all select 9,'z',2,0x559c2edab7b8;
insert into calls values (17,0x559c2edb1e00,'j','<console>',0,0x559c2edab7b8);
insert into promises select 0x559c2edb1ff8,17,7 union all select 0x559c2edb1f88,17,8 union all select 0x559c2edb1f18,17,9;
insert into calls values (18,0x559c2edb1e00,'+',NULL,1,0x559c2d7f1780);
insert into promise_evaluations values ($next_id,0xf,0x559c2edb1ff8,17);
insert into calls values (19,0x559c2edb1c08,'h','<console>',0,0x559c2edaa388);
insert into promises select 0x559c2edb1d58,19,5 union all select 0x559c2edb1ce8,19,6;
insert into calls values (20,0x559c2edb1c08,'{',NULL,1,0x559c2d7ee9a8);
insert into calls values (21,0x559c2edb1c08,'+',NULL,1,0x559c2d7f1780);
insert into calls values (22,0x559c2edb1c08,'+',NULL,1,0x559c2d7f1780);
insert into promise_evaluations values ($next_id,0xf,0x559c2edb1d58,19);
insert into promise_evaluations values ($next_id,0xf,0x559c2edb1f18,17);
insert into promise_evaluations values ($next_id,0xf,0x559c2edb1ce8,19);
insert into promise_evaluations values ($next_id,0x0,0x559c2edb1f18,17);
insert into calls values (23,0x559c2edb27f0,'j','<console>',0,0x559c2edab7b8);
insert into promises select 0x559c2edb29e8,23,7 union all select 0x559c2edb2978,23,8 union all select 0x559c2edb2908,23,9;
insert into calls values (24,0x559c2edb27f0,'+',NULL,1,0x559c2d7f1780);
insert into promise_evaluations values ($next_id,0xf,0x559c2edb29e8,23);
insert into calls values (25,0x559c2edb25f8,'h','<console>',0,0x559c2edaa388);
insert into promises select 0x559c2edb2748,25,5 union all select 0x559c2edb26d8,25,6;
insert into calls values (26,0x559c2edb25f8,'{',NULL,1,0x559c2d7ee9a8);
insert into calls values (27,0x559c2edb25f8,'+',NULL,1,0x559c2d7f1780);
insert into calls values (28,0x559c2edb25f8,'+',NULL,1,0x559c2d7f1780);
insert into promise_evaluations values ($next_id,0xf,0x559c2edb2748,25);
insert into promise_evaluations values ($next_id,0xf,0x559c2edb2908,23);
insert into promise_evaluations values ($next_id,0xf,0x559c2edb26d8,25);
insert into promise_evaluations values ($next_id,0x0,0x559c2edb2908,23);
insert into functions values (0x559c2edaa7c8,'<console>','function (x, y, ...) 
{
    h(v = x, w = y) + h(...)
}');
insert into arguments select 10,'x',0,0x559c2edaa7c8 union all select 11,'y',1,0x559c2edaa7c8 union all select 12,'...',2,0x559c2edaa7c8;
insert into calls values (29,0x559c2edb30c8,'i','<console>',0,0x559c2edaa7c8);
insert into promises select 0x559c2edb2438,29,10 union all select 0x559c2edb23c8,29,11 union all select 0x559c2edb2358,29,12 union all select 0x559c2edb22e8,29,12;
insert into calls values (30,0x559c2edb30c8,'{',NULL,1,0x559c2d7ee9a8);
insert into calls values (31,0x559c2edb30c8,'+',NULL,1,0x559c2d7f1780);
insert into calls values (32,0x559c2edb2f08,'h','<console>',0,0x559c2edaa388);
insert into promises select 0x559c2edb3058,32,5 union all select 0x559c2edb2fe8,32,6;
insert into calls values (33,0x559c2edb2f08,'{',NULL,1,0x559c2d7ee9a8);
insert into calls values (34,0x559c2edb2f08,'+',NULL,1,0x559c2d7f1780);
insert into calls values (35,0x559c2edb2f08,'+',NULL,1,0x559c2d7f1780);
insert into promise_evaluations values ($next_id,0xf,0x559c2edb3058,32);
insert into promise_evaluations values ($next_id,0xf,0x559c2edb2438,29);
insert into promise_evaluations values ($next_id,0xf,0x559c2edb2fe8,32);
insert into promise_evaluations values ($next_id,0xf,0x559c2edb23c8,29);
insert into calls values (36,0x559c2edb2c30,'h','<console>',0,0x559c2edaa388);
insert into promises select 0x559c2edb2d80,36,5 union all select 0x559c2edb2d10,36,6;
insert into calls values (37,0x559c2edb2c30,'{',NULL,1,0x559c2d7ee9a8);
insert into calls values (38,0x559c2edb2c30,'+',NULL,1,0x559c2d7f1780);
insert into calls values (39,0x559c2edb2c30,'+',NULL,1,0x559c2d7f1780);
insert into promise_evaluations values ($next_id,0xf,0x559c2edb2d80,36);
insert into promise_evaluations values ($next_id,0xf,0x559c2edb2358,29);
insert into promise_evaluations values ($next_id,0xf,0x559c2edb2d10,36);
insert into promise_evaluations values ($next_id,0xf,0x559c2edb22e8,29);
