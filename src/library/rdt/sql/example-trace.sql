-- f <- function(x) x
-- g <- function(x,y,z) {x+y+z}
-- h <- function(v,w) {v+1+w}
-- i <- function(x,y,...) {h(v=x,w=y) + h(...)}
-- j <- function(x,y,z) x + h(v=z,w=z)
-- k <- function(...) { l<-list(...); f(l) }
-- l <- function(...) { l<-list(...); print(l) }
-- trace.promises.sql({f(1); f(2); f(3); f(4); g(1+1,2+2,3+3); h(1,1); j(1,1,1); j(1,1,1); i(1,1,1,1); l(1,2,3); k(1,2,3);})

insert into promise_evaluations values ($next_id,0xf,0x56052cadbdf0,0);
insert into promise_evaluations values ($next_id,0xf,0x56052caeacf0,0);
insert into functions values (0x56052bafc9a8,NULL,NULL);
insert into calls values (1,0x56052bb27428,'{',NULL,1,0x56052bafc9a8);
insert into functions values (0x56052c7936d8,NULL,'function (x) 
x');
insert into arguments select 1,'x',0,0x56052c7936d8;
insert into calls values (2,0x56052cae1250,'f',NULL,0,0x56052c7936d8);
insert into promises select 0x56052cae12f8;
insert into promise_associations select 0x56052cae12f8,2,1;
insert into promise_evaluations values ($next_id,0xf,0x56052cae12f8,2);
insert into calls values (3,0x56052cae20a0,'f',NULL,0,0x56052c7936d8);
insert into promises select 0x56052cae11e0;
insert into promise_associations select 0x56052cae11e0,3,1;
insert into promise_evaluations values ($next_id,0xf,0x56052cae11e0,3);
insert into calls values (4,0x56052cae1f88,'f',NULL,0,0x56052c7936d8);
insert into promises select 0x56052cae2030;
insert into promise_associations select 0x56052cae2030,4,1;
insert into promise_evaluations values ($next_id,0xf,0x56052cae2030,4);
insert into calls values (5,0x56052cae1e70,'f',NULL,0,0x56052c7936d8);
insert into promises select 0x56052cae1f18;
insert into promise_associations select 0x56052cae1f18,5,1;
insert into promise_evaluations values ($next_id,0xf,0x56052cae1f18,5);
insert into functions values (0x56052c79f838,NULL,'function (x, y, z) 
{
    x + y + z
}');
insert into arguments select 2,'x',0,0x56052c79f838 union all select 3,'y',1,0x56052c79f838 union all select 4,'z',2,0x56052c79f838;
insert into calls values (6,0x56052cae1bd0,'g',NULL,0,0x56052c79f838);
insert into promises select 0x56052cae1dc8 union all select 0x56052cae1d58 union all select 0x56052cae1ce8;
insert into promise_associations select 0x56052cae1dc8,6,2 union all select 0x56052cae1d58,6,3 union all select 0x56052cae1ce8,6,4;
insert into calls values (7,0x56052cae1bd0,'{',NULL,1,0x56052bafc9a8);
insert into functions values (0x56052baff780,NULL,NULL);
insert into calls values (8,0x56052cae1bd0,'+',NULL,1,0x56052baff780);
insert into calls values (9,0x56052cae1bd0,'+',NULL,1,0x56052baff780);
insert into promise_evaluations values ($next_id,0xf,0x56052cae1dc8,6);
insert into calls values (10,0x56052bb27428,'+',NULL,1,0x56052baff780);
insert into promise_evaluations values ($next_id,0xf,0x56052cae1d58,6);
insert into calls values (11,0x56052bb27428,'+',NULL,1,0x56052baff780);
insert into promise_evaluations values ($next_id,0xf,0x56052cae1ce8,6);
insert into calls values (12,0x56052bb27428,'+',NULL,1,0x56052baff780);
insert into functions values (0x56052c79f368,NULL,'function (v, w) 
{
    v + 1 + w
}');
insert into arguments select 5,'v',0,0x56052c79f368 union all select 6,'w',1,0x56052c79f368;
insert into calls values (13,0x56052cae26d8,'h',NULL,0,0x56052c79f368);
insert into promises select 0x56052cae2828 union all select 0x56052cae27b8;
insert into promise_associations select 0x56052cae2828,13,5 union all select 0x56052cae27b8,13,6;
insert into calls values (14,0x56052cae26d8,'{',NULL,1,0x56052bafc9a8);
insert into calls values (15,0x56052cae26d8,'+',NULL,1,0x56052baff780);
insert into calls values (16,0x56052cae26d8,'+',NULL,1,0x56052baff780);
insert into promise_evaluations values ($next_id,0xf,0x56052cae2828,13);
insert into promise_evaluations values ($next_id,0xf,0x56052cae27b8,13);
insert into functions values (0x56052c7a0630,NULL,'function (x, y, z) 
x + h(v = z, w = z)');
insert into arguments select 7,'x',0,0x56052c7a0630 union all select 8,'y',1,0x56052c7a0630 union all select 9,'z',2,0x56052c7a0630;
insert into calls values (17,0x56052cae2358,'j',NULL,0,0x56052c7a0630);
insert into promises select 0x56052cae2550 union all select 0x56052cae24e0 union all select 0x56052cae2470;
insert into promise_associations select 0x56052cae2550,17,7 union all select 0x56052cae24e0,17,8 union all select 0x56052cae2470,17,9;
insert into calls values (18,0x56052cae2358,'+',NULL,1,0x56052baff780);
insert into promise_evaluations values ($next_id,0xf,0x56052cae2550,17);
insert into calls values (19,0x56052cae2160,'h',NULL,0,0x56052c79f368);
insert into promises select 0x56052cae22b0 union all select 0x56052cae2240;
insert into promise_associations select 0x56052cae22b0,19,5 union all select 0x56052cae2240,19,6;
insert into calls values (20,0x56052cae2160,'{',NULL,1,0x56052bafc9a8);
insert into calls values (21,0x56052cae2160,'+',NULL,1,0x56052baff780);
insert into calls values (22,0x56052cae2160,'+',NULL,1,0x56052baff780);
insert into promise_evaluations values ($next_id,0xf,0x56052cae22b0,19);
insert into promise_evaluations values ($next_id,0xf,0x56052cae2470,17);
insert into promise_evaluations values ($next_id,0xf,0x56052cae2240,19);
insert into promise_evaluations values ($next_id,0x0,0x56052cae2470,17);
insert into calls values (23,0x56052cae2d48,'j',NULL,0,0x56052c7a0630);
insert into promises select 0x56052cae2f40 union all select 0x56052cae2ed0 union all select 0x56052cae2e60;
insert into promise_associations select 0x56052cae2f40,23,7 union all select 0x56052cae2ed0,23,8 union all select 0x56052cae2e60,23,9;
insert into calls values (24,0x56052cae2d48,'+',NULL,1,0x56052baff780);
insert into promise_evaluations values ($next_id,0xf,0x56052cae2f40,23);
insert into calls values (25,0x56052cae2b50,'h',NULL,0,0x56052c79f368);
insert into promises select 0x56052cae2ca0 union all select 0x56052cae2c30;
insert into promise_associations select 0x56052cae2ca0,25,5 union all select 0x56052cae2c30,25,6;
insert into calls values (26,0x56052cae2b50,'{',NULL,1,0x56052bafc9a8);
insert into calls values (27,0x56052cae2b50,'+',NULL,1,0x56052baff780);
insert into calls values (28,0x56052cae2b50,'+',NULL,1,0x56052baff780);
insert into promise_evaluations values ($next_id,0xf,0x56052cae2ca0,25);
insert into promise_evaluations values ($next_id,0xf,0x56052cae2e60,23);
insert into promise_evaluations values ($next_id,0xf,0x56052cae2c30,25);
insert into promise_evaluations values ($next_id,0x0,0x56052cae2e60,23);
insert into functions values (0x56052c79fc08,NULL,'function (x, y, ...) 
{
    h(v = x, w = y) + h(...)
}');
insert into arguments select 10,'x',0,0x56052c79fc08 union all select 11,'y',1,0x56052c79fc08 union all select 12,'...[0]',2,0x56052c79fc08 union all select 13,'...[1]',3,0x56052c79fc08;
insert into calls values (29,0x56052cae3620,'i',NULL,0,0x56052c79fc08);
insert into promises select 0x56052cae2990 union all select 0x56052cae2920 union all select 0x56052cae3818 union all select 0x56052cae37a8;
insert into promise_associations select 0x56052cae2990,29,10 union all select 0x56052cae2920,29,11 union all select 0x56052cae3818,29,12 union all select 0x56052cae37a8,29,13;
insert into calls values (30,0x56052cae3620,'{',NULL,1,0x56052bafc9a8);
insert into calls values (31,0x56052cae3620,'+',NULL,1,0x56052baff780);
insert into calls values (32,0x56052cae3460,'h',NULL,0,0x56052c79f368);
insert into promises select 0x56052cae35b0 union all select 0x56052cae3540;
insert into promise_associations select 0x56052cae35b0,32,5 union all select 0x56052cae3540,32,6;
insert into calls values (33,0x56052cae3460,'{',NULL,1,0x56052bafc9a8);
insert into calls values (34,0x56052cae3460,'+',NULL,1,0x56052baff780);
insert into calls values (35,0x56052cae3460,'+',NULL,1,0x56052baff780);
insert into promise_evaluations values ($next_id,0xf,0x56052cae35b0,32);
insert into promise_evaluations values ($next_id,0xf,0x56052cae2990,29);
insert into promise_evaluations values ($next_id,0xf,0x56052cae3540,32);
insert into promise_evaluations values ($next_id,0xf,0x56052cae2920,29);
insert into calls values (36,0x56052cae3188,'h',NULL,0,0x56052c79f368);
insert into promises select 0x56052cae32d8 union all select 0x56052cae3268;
insert into promise_associations select 0x56052cae32d8,36,5 union all select 0x56052cae3268,36,6;
insert into calls values (37,0x56052cae3188,'{',NULL,1,0x56052bafc9a8);
insert into calls values (38,0x56052cae3188,'+',NULL,1,0x56052baff780);
insert into calls values (39,0x56052cae3188,'+',NULL,1,0x56052baff780);
insert into promise_evaluations values ($next_id,0xf,0x56052cae32d8,36);
insert into promise_evaluations values ($next_id,0xf,0x56052cae3818,29);
insert into promise_evaluations values ($next_id,0xf,0x56052cae3268,36);
insert into promise_evaluations values ($next_id,0xf,0x56052cae37a8,29);
insert into functions values (0x56052c7a0f40,NULL,'function (...) 
{
    l <- list(...)
    f(l)
}');
insert into arguments select 14,'...[0]',0,0x56052c7a0f40 union all select 15,'...[1]',1,0x56052c7a0f40 union all select 16,'...[2]',2,0x56052c7a0f40;
insert into calls values (40,0x56052cae3d00,'k',NULL,0,0x56052c7a0f40);
insert into promises select 0x56052cae3f30 union all select 0x56052cae3ec0 union all select 0x56052cae3e50;
insert into promise_associations select 0x56052cae3f30,40,14 union all select 0x56052cae3ec0,40,15 union all select 0x56052cae3e50,40,16;
insert into calls values (41,0x56052cae3d00,'{',NULL,1,0x56052bafc9a8);
insert into functions values (0x56052baf9df8,NULL,NULL);
insert into calls values (42,0x56052cae3d00,'<-',NULL,1,0x56052baf9df8);
insert into functions values (0x56052bb113a0,NULL,NULL);
insert into calls values (43,0x56052cae3d00,'list',NULL,1,0x56052bb113a0);
insert into promise_evaluations values ($next_id,0xf,0x56052cae3f30,40);
insert into promise_evaluations values ($next_id,0xf,0x56052cae3ec0,40);
insert into promise_evaluations values ($next_id,0xf,0x56052cae3e50,40);
insert into calls values (44,0x56052cae3ad0,'f',NULL,0,0x56052c7936d8);
insert into promises select 0x56052cae3b78;
insert into promise_associations select 0x56052cae3b78,44,1;
insert into promise_evaluations values ($next_id,0xf,0x56052cae3b78,44);
insert into promise_evaluations values ($next_id,0xf,0x56052bbde168,0);
insert into functions values (0x56052bb1ea00,NULL,NULL);
insert into calls values (46,0x56052bb6b438,'lazyLoadDBfetch',NULL,1,0x56052bb1ea00);
insert into functions values (0x56052c7a0958,NULL,'function (...) 
{
    l <- list(...)
    print(l)
}');
insert into arguments select 17,'...[0]',0,0x56052c7a0958 union all select 18,'...[1]',1,0x56052c7a0958 union all select 19,'...[2]',2,0x56052c7a0958;
insert into calls values (45,0x56052cae4760,'l',NULL,0,0x56052c7a0958);
insert into promises select 0x56052cae3a28 union all select 0x56052cae39b8 union all select 0x56052cae3948;
insert into promise_associations select 0x56052cae3a28,45,17 union all select 0x56052cae39b8,45,18 union all select 0x56052cae3948,45,19;
insert into calls values (47,0x56052cae4760,'{',NULL,1,0x56052bafc9a8);
insert into calls values (48,0x56052cae4760,'<-',NULL,1,0x56052baf9df8);
insert into calls values (49,0x56052cae4760,'list',NULL,1,0x56052bb113a0);
insert into promise_evaluations values ($next_id,0xf,0x56052cae3a28,45);
insert into promise_evaluations values ($next_id,0xf,0x56052cae39b8,45);
insert into promise_evaluations values ($next_id,0xf,0x56052cae3948,45);
insert into functions values (0x56052cae4610,NULL,'function (x, ...) 
UseMethod("print")');
insert into arguments select 20,'x',0,0x56052cae4610;
insert into calls values (50,0x56052cae4290,'print',NULL,0,0x56052cae4610);
insert into promises select 0x56052cae4370;
insert into promise_associations select 0x56052cae4370,50,20;
insert into promise_evaluations values ($next_id,0x30,0x56052cae4370,50); -- manually edited
insert into promise_evaluations values ($next_id,0xf,0x56052bbdf790,0);
insert into calls values (51,0x56052bb6b438,'lazyLoadDBfetch',NULL,1,0x56052bb1ea00);
insert into functions values (0x56052cae4060,NULL,'function (x, digits = NULL, quote = TRUE, na.print = NULL, print.gap = NULL, right = FALSE, max = NULL, useSource = TRUE, ...) 
{
    noOpt <- missing(digits) && missing(quote) && missing(na.print) && missing(print.gap) && missing(right) && missing(max) && missing(useSource) && missing(...)
    .Internal(print.default(x, digits, quote, na.print, print.gap, right, max, useSource, noOpt))
}');
insert into arguments select 21,'x',0,0x56052cae4060 union all select 22,'digits',1,0x56052cae4060 union all select 23,'quote',2,0x56052cae4060 union all select 24,'na.print',3,0x56052cae4060 union all select 25,'print.gap',4,0x56052cae4060 union all select 26,'right',5,0x56052cae4060 union all select 27,'max',6,0x56052cae4060 union all select 28,'useSource',7,0x56052cae4060;
insert into calls values (52,0x56052cae71f0,'print.default',NULL,0,0x56052cae4060);
insert into promises select 0x56052cae71b8 union all select 0x56052cae7180 union all select 0x56052cae7148 union all select 0x56052cae7110 union all select 0x56052cae70d8 union all select 0x56052cae70a0 union all select 0x56052cae7068;
insert into promise_associations select 0x56052cae4370,52,21 union all select 0x56052cae71b8,52,22 union all select 0x56052cae7180,52,23 union all select 0x56052cae7148,52,24 union all select 0x56052cae7110,52,25 union all select 0x56052cae70d8,52,26 union all select 0x56052cae70a0,52,27 union all select 0x56052cae7068,52,28;
insert into promise_evaluations values ($next_id,0xf,0x56052cae4370,52);
insert into promise_evaluations values ($next_id,0xf,0x56052cae71b8,52);
insert into promise_evaluations values ($next_id,0xf,0x56052cae7180,52);
insert into promise_evaluations values ($next_id,0xf,0x56052cae7148,52);
insert into promise_evaluations values ($next_id,0xf,0x56052cae7110,52);
insert into promise_evaluations values ($next_id,0xf,0x56052cae70d8,52);
insert into promise_evaluations values ($next_id,0xf,0x56052cae70a0,52);
insert into promise_evaluations values ($next_id,0xf,0x56052cae7068,52);
insert into functions values (0x56052bb139b8,NULL,NULL);
insert into calls values (53,0x56052cae71f0,'print.default',NULL,1,0x56052bb139b8);
