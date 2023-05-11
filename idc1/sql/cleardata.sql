delete from T_ZHOBTMIND  where ddatetime<timestampadd(minute,-120,now());
delete from T_ZHOBTMIND1 where ddatetime<timestampadd(minute,-120,now());