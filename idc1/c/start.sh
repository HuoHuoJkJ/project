#############################
# 启动数据中心后台服务程序的脚本 #
#############################

# 检查服务程序是否超时，配置在/etc/rc.local中由root用户执行。
# /project/tools1/bin/procctl 30 /project/tools1/bin/checkproc

# 压缩数据中心后台服务程序的备份日志。
/project/tools1/bin/procctl 300 /project/tools1/bin/gzipfiles /log/idc "*.log.20*" 0.04

# 生成用于测试的全国气象站点观测的分钟数据到/tmp/idc/surfdata
/project/tools1/bin/procctl 60 /project/idc1/bin/crtsurfdata /project/idc/ini/stcode.ini /tmp/idc/surfdata /log/idc/crtsurfdata.log xml,json,csv

# 清理原始的全国气象站点观测的分钟数据目录/tmp/idc/surfdata中的历史数据文件
/project/tools1/bin/procctl 300 /project/tools1/bin/deletefiles /tmp/idc/surfdata "*" 0.04

# 采集全国气象站点的分钟观测数据的xml文件存放到/idcdata/surfdata
/project/tools1/bin/procctl 30 /project/tools1/bin/ftpgetfiles /log/idc/ftpgetfiles_surfdata.log "<host>127.0.0.1:21</host><mode>1</mode><username>lighthouse</username><password>dyt.9525</password><localpath>/idcdata/surfdata</localpath><remotepath>/tmp/idc/surfdata</remotepath><matchname>SURF_ZH*.XML</matchname><listfilename>/idcdata/ftplist/ftpgetfiles_surfdata.list</listfilename><ptype>1</ptype><okfilename>/idcdata/ftplist/ftpgetfiles_surfdata.xml</okfilename><checktime>true</checktime><timeout>80</timeout><pname>ftpgetfiles_surfdata</pname>"

# 清理原始的全国气象站点观测的分钟数据目录/tmp/idc/surfdata中的历史数据文件
/project/tools1/bin/procctl 300 /project/tools1/bin/deletefiles /idcdata/surfdata "*" 0.04

# 上传全国气象站点的分钟观测数据的json文件 到目录/tmp/ftpputest
/project/tools1/bin/procctl 30 /project/tools1/bin/ftpputfiles /log/idc/ftpputfiles_surfdata.log "<host>127.0.0.1:21</host><mode>1</mode><username>lighthouse</username><password>dyt.9525</password><localpath>/tmp/idc/surfdata</localpath><remotepath>/tmp/ftpputest</remotepath><matchname>SURF_ZH*.JSON</matchname><ptype>1</ptype><localpathbak>/tmp/idc/surfdatabak</localpathbak><okfilename>/idcdata/ftplist/ftpputfiles_surfdata.xml</okfilename><timeout>80</timeout><pname>ftpputfiles_surfdata</pname>"

# 清理原始的全国气象站点观测的分钟数据目录/tmp/idc/surfdata中的历史数据文件
/project/tools1/bin/procctl 300 /project/tools1/bin/deletefiles /tmp/ftpputest "*" 0.04

# 文件传输的服务端程序
/project/tools1/bin/procctl 10 /project/tools1/bin/fileserver /log/idc/fileserver.log 5005

# 把目录/tmp/ftpputest中的文件上传到/tmp/tcpputest目录中
/project/tools1/bin/procctl 20 /project/tools1/bin/tcpputfiles /log/idc/tcpputfiles.log "<ip>127.0.0.1</ip><port>5005</port><ptype>1</ptype><clientpath>/tmp/ftpputest</clientpath><serverpath>/tmp/tcpputest</serverpath><clientpathbak>/tmp/tcp/clientbak</clientpathbak><matchname>*.JSON</matchname><andchild>true</andchild><timetvl>10</timetvl><timeout>50</timeout><pname>tcpputfiles</pname>"

# 把目录/tmp/tcpputest中的文件下载到/tmp/tcpgetest目录中
/project/tools1/bin/procctl 20 /project/tools1/bin/tcpgetfiles /log/idc/tcpgetfiles.log "<ip>127.0.0.1</ip><port>5005</port><ptype>1</ptype><clientpath>/tmp/tcpgetest</clientpath><serverpath>/tmp/tcpputest</serverpath><serverpathbak>/tmp/tcp/serverbak</serverpathbak><matchname>*.JSON</matchname><andchild>true</andchild><timetvl>10</timetvl><timeout>50</timeout><pname>tcpgetfiles</pname>"

# 清理采集的全国气象站点观测的分钟数据目录/tmp/tcpgtest中的历史数据文件
/project/tools1/bin/procctl 300 /project/tools1/bin/deletefiles /tmp/tcpgetest "*" 0.04

# 将站点参数文件内容入库，如果站点内容不在库中，则插入；如果站点内容以修改，则更新
/project/tools1/bin/procctl 120 /project/idc1/bin/obtcodetodb /project/idc/ini/stcode.ini "127.0.0.1,root,DYT.9525ing,TestDB,3306" utf8 /log/idc/obtcodetodb.log

# 把全国站点分钟观测数据保存到数据库的T_ZHOBTMIND表中，数据只插入，不更新
/project/tools1/bin/procctl 10 /project/idc1/bin/obtmindtodb /idcdata/surfdata "127.0.0.1,root,DYT.9525ing,TestDB,3306" utf8 /log/idc/obtmindtodb.log

# 清理T_ZHOBTMIND表中120分之前的数据，防止磁盘空间被撑满
# /project/tools1/bin/procctl 120 /project/tools1/bin/execsql /project/idc1/sql/cleardata.sql "127.0.0.1,root,DYT.9525ing,TestDB,3306" utf8 /log/idc/execsql.log
# /project/tools1/bin/procctl 120 /project/tools1/bin/execsql /project/idc1/sql/cleardata1.sql "127.0.0.1,root,DYT.9525ing,TestDB1,3306" utf8 /log/idc/execsql.log

# 每3600秒从数据源数据库 全量抽取 一次存放站点参数的表的数据 保存到 目录/idcdata/dmindata下
/project/tools1/bin/procctl 3600 /project/tools1/bin/dminingmysql /log/idc/dminingmysql_ZHOBTCODE.log "<connstr>127.0.0.1,root,DYT.9525ing,TestDB,3306</connstr><charaset>utf8</charaset><selectsql>select obtid,cityname,provname,lat,lon,height from T_ZHOBTCODE</selectsql><fieldstr>obtid,cityname,provname,lat,lon,height</fieldstr><fieldlen>10,30,30,10,10,10</fieldlen><bfilename>ZHOBTCODE</bfilename><efilename>HYCZ</efilename><outpath>/idcdata/dmindata</outpath><timeout>30</timeout><pname>dminingmysq1_ZHOBTMIND</pname>"

# 每30秒从数据源数据库 增量抽取 一次存放观测参数的表的数据 保存到 目录/idcdata/dmindata下
/project/tools1/bin/procctl 30 /project/tools1/bin/dminingmysql /log/idc/dminingmysql_ZHOBTMIND.log "<connstr>127.0.0.1,root,DYT.9525ing,TestDB,3306</connstr><charaset>utf8</charaset><selectsql>select obtid,date_format(ddatetime,'%%Y-%%m-%%d %%H:%%m:%%s'),t,p,u,wd,wf,r,vis,keyid from T_ZHOBTMIND where keyid>:1 and ddatetime>timestampadd(minute,-120,now())</selectsql><fieldstr>obtid,ddatetime,t,p,u,wd,wf,r,vis,keyid</fieldstr><fieldlen>10,19,8,8,8,8,8,8,8,15</fieldlen><bfilename>ZHOBTMIND</bfilename><efilename>HYCZ</efilename><outpath>/idcdata/dmindata</outpath><maxcount>1000</maxcount><incfield>keyid</incfield><connstr1>127.0.0.1,root,DYT.9525ing,TestDB,3306</connstr1><timeout>30</timeout><pname>dminingmysq1_ZHOBTMIND_HYCZ</pname>"

# 清理采集的全国气象站点观测的分钟数据目录 /idcdata/dmindata 中的历史数据文件
/project/tools1/bin/procctl 300 /project/tools1/bin/deletefiles /idcdata/dmindata "*" 0.04

# 将数据抽取进程dminingmysql抽取到/idcdata/dmindata的文件，上传到/idcdata/xmltodb/vip1当中，进行数据入库的操作
/project/tools1/bin/procctl 20 /project/tools1/bin/tcpputfiles /log/idc/tcpputfiles_dmindata.log "<ip>127.0.0.1</ip><port>5005</port><ptype>1</ptype><clientpath>/idcdata/dmindata</clientpath><serverpath>/idcdata/xmltodb/vip1</serverpath><matchname>*.XML</matchname><andchild>true</andchild><timetvl>10</timetvl><timeout>50</timeout><pname>tcpputfiles_dmindata</pname>"

# 启动将xml文件入库的程序，将/idcdata/xmltodb/vip1目录下的xml文件数据入库
/project/tools1/bin/procctl 10 /project/tools1/bin/xmltodb /log/idc/xmltodb_vip1.log "<connstr>127.0.0.1,root,DYT.9525ing,TestDB,3306</connstr><charaset>utf8</charaset><inifilename>/project/tools/ini/xmltodb.xml</inifilename><xmlpath>/idcdata/xmltodb/vip1</xmlpath><xmlpathbak>/idcdata/xmltodb/vip1bak</xmlpathbak><xmlpatherr>/idcdata/xmltodb/vip1err</xmlpatherr><timetvl>5</timetvl><timeout>50</timeout><pname>xmltodb_vip1</pname>"

# 清理/idcdata/xmltodb/vip1bak 和 /idcdata/xmltodb/vip1err目录下的文件
/project/tools1/bin/procctl 300 /project/tools1/bin/deletefiles /idcdata/xmltodb/vip1bak "*" 0.04
/project/tools1/bin/procctl 300 /project/tools1/bin/deletefiles /idcdata/xmltodb/vip1err "*" 0.04

# 采用全表刷新的方法，把表T_ZHOBTCODE1中的数据同步到表T_ZHOBTCODE2中
/project/tools1/bin/procctl 10 /project/tools1/bin/syncupdate /log/idc/syncupdate_ZHOBTCODE2.log "<localconnstr>127.0.0.1,root,DYT.9525ing,TestDB1,3306</localconnstr><charset>utf8</charset><fedtname>LK_ZHOBTCODE1</fedtname><localtname>T_ZHOBTCODE2</localtname><remotecols>obtid,cityname,provname,lat,lon,height/10,upttime,keyid</remotecols><localcols>stid,cityname,provname,lat,lon,altitude,upttime,id</localcols><synctype>1</synctype><timeout>50</timeout><pname>syncupdate_ZHOBTCODE2</pname>"

# 采用分批刷新的方法，把字段符合obtid like '54%'的表T_ZHOBTCODE1中的数据同步到表T_ZHOBTCODE3中
/project/tools1/bin/procctl 10 /project/tools1/bin/syncupdate /log/idc/syncupdate_ZHOBTCODE3.log "<localconnstr>127.0.0.1,root,DYT.9525ing,TestDB1,3306</localconnstr><charset>utf8</charset><fedtname>LK_ZHOBTCODE1</fedtname><localtname>T_ZHOBTCODE3</localtname><remotecols>obtid,cityname,provname,lat,lon,height/10,upttime,keyid</remotecols><localcols>stid,cityname,provname,lat,lon,altitude,upttime,id</localcols><where>where obtid like '54%%'</where><synctype>2</synctype><remoteconnstr>127.0.0.1,root,DYT.9525ing,TestDB,3306</remoteconnstr><remotetname>T_ZHOBTCODE1</remotetname><remotekeycol>obtid</remotekeycol><localkeycol>stid</localkeycol><maxcount>10</maxcount><timeout>50</timeout><pname>syncupdate_ZHOBTCODE3</pname>"

# 采用增量同步的方法，把表T_ZHOBTMIND1中的数据同步到表T_ZHOBTMIND2中
/project/tools1/bin/procctl 10 /project/tools1/bin/syncincrement /log/idc/syncincrement_ZHOBTMIND2.log "<localconnstr>127.0.0.1,root,DYT.9525ing,TestDB1,3306</localconnstr><remoteconnstr>127.0.0.1,root,DYT.9525ing,TestDB,3306</remoteconnstr><charset>utf8</charset><remotetname>T_ZHOBTMIND1</remotetname><fedtname>LK_ZHOBTMIND1</fedtname><localtname>T_ZHOBTMIND2</localtname><remotecols>obtid,ddatetime,t,p,u,wd,wf,r,vis,upttime,keyid</remotecols><localcols>stid,ddatetime,t,p,u,wd,wf,r,vis,upttime,recid</localcols><remotekeycol>keyid</remotekeycol><localkeycol>recid</localkeycol><maxcount>300</maxcount><timetvl>2</timetvl><timeout>50</timeout><pname>syncincrement1_ZHOBTMIND2</pname>"

# 采用增量同步的方法，把字段符合obtid like '54%'的表T_ZHOBTMIND1中的数据同步到表T_ZHOBTMIND3中
/project/tools1/bin/procctl 10 /project/tools1/bin/syncincrement /log/idc/syncincrement_ZHOBTMIND3.log "<localconnstr>127.0.0.1,root,DYT.9525ing,TestDB1,3306</localconnstr><remoteconnstr>127.0.0.1,root,DYT.9525ing,TestDB,3306</remoteconnstr><charset>utf8</charset><remotetname>T_ZHOBTMIND1</remotetname><fedtname>LK_ZHOBTMIND1</fedtname><localtname>T_ZHOBTMIND3</localtname><remotecols>obtid,ddatetime,t,p,u,wd,wf,r,vis,upttime,keyid</remotecols><localcols>stid,ddatetime,t,p,u,wd,wf,r,vis,upttime,recid</localcols><where>and obtid like '54%'</where><remotekeycol>keyid</remotekeycol><localkeycol>recid</localkeycol><maxcount>300</maxcount><timetvl>2</timetvl><timeout>50</timeout><pname>syncincrement1_ZHOBTMIND2</pname>"

# 把T_ZHOBTMIND表中120分钟之前的数据迁移到T_ZHOBTMIND_HIS表中
/project/tools1/bin/procctl 3600 /project/tools1/bin/migratetabledata /log/idc/migratetabledata_T_ZHOBTMIND.log "<connectstr>127.0.0.1,root,DYT.9525ing,TestDB,3306</connectstr><srctname>T_ZHOBTMIND</srctname><dsttname>T_ZHOBTMIND_HIS</dsttname><keycol>keyid</keycol><where>where ddatetime<timestampadd(minute,-120,now())</where><starttime>01,02,03,04,05,13</starttime><maxcount>300</maxcount><timeout>120</timeout><pname>migratetabledata_ZHOBTMIND1</pname>"

# 把T_ZHOBTMIND1表中120分钟之前的数据清除
/project/tools1/bin/procctl 3600 /project/tools1/bin/deletetabledata /log/idc/deletetabledata_T_ZHOBTMIND1.log "<connectstr>127.0.0.1,root,DYT.9525ing,TestDB,3306</connectstr><tname>T_ZHOBTMIND1</tname><keycol>keyid</keycol><where>where ddatetime<timestampadd(minute,-120,now())</where><starttime>01,02,03,04,05,13</starttime><timeout>120</timeout><pname>deletetabledata_ZHOBTMIND1</pname>"

# 把T_ZHOBTMIND2表中120分钟之前的数据清除
/project/tools1/bin/procctl 3600 /project/tools1/bin/deletetabledata /log/idc/deletetabledata_T_ZHOBTMIND2.log "<connectstr>127.0.0.1,root,DYT.9525ing,TestDB1,3306</connectstr><tname>T_ZHOBTMIND2</tname><keycol>recid</keycol><where>where ddatetime<timestampadd(minute,-120,now())</where><starttime>01,02,03,04,05,13</starttime><timeout>120</timeout><pname>deletetabledata_ZHOBTMIND2</pname>"

# 把T_ZHOBTMIND3表中120分钟之前的数据清除
/project/tools1/bin/procctl 3600 /project/tools1/bin/deletetabledata /log/idc/deletetabledata_T_ZHOBTMIND3.log "<connectstr>127.0.0.1,root,DYT.9525ing,TestDB1,3306</connectstr><tname>T_ZHOBTMIND3</tname><keycol>recid</keycol><where>where ddatetime<timestampadd(minute,-120,now())</where><starttime>01,02,03,04,05,13</starttime><timeout>120</timeout><pname>deletetabledata_ZHOBTMIND3</pname>"

# 把T_ZHOBTMIND_HIS表中240分钟之前的数据清除
/project/tools1/bin/procctl 3600 /project/tools1/bin/deletetabledata /log/idc/deletetabledata_T_ZHOBTMIND_HIS.log "<connectstr>127.0.0.1,root,DYT.9525ing,TestDB,3306</connectstr><tname>T_ZHOBTMIND_HIS</tname><keycol>keyid</keycol><where>where ddatetime<timestampadd(minute,-240,now())</where><starttime>01,02,03,04,05,13</starttime><timeout>120</timeout><pname>deletetabledata_ZHOBTMIND_HIS</pname>"

# 每隔一小时把mysql数据库的T_ZHOBTCODE表中全部的数据抽取出来，放在/idcdata/xmltodb/vip2目录，给xmltodb_oracle入库到Oracle
/project/tools1/bin/procctl 3600 /project/tools1/bin/dminingmysql /log/idc/dminingmysql_ZHOBTCODE_vip2.log "<connstr>127.0.0.1,root,DYT.9525ing,TestDB,3306</connstr><charaset>utf8</charaset><selectsql>select obtid,cityname,provname,lat,lon,height from T_ZHOBTCODE</selectsql><fieldstr>obtid,cityname,provname,lat,lon,height</fieldstr><fieldlen>10,30,30,10,10,10</fieldlen><bfilename>ZHOBTCODE</bfilename><efilename>HYCZ</efilename><outpath>/idcdata/xmltodb/vip2</outpath><timeout>30</timeout><pname>dminingmysql_ZHOBTCODE_vip2</pname>"

# 每隔一小时把mysql数据库的T_ZHOBTMIND表中全部的数据抽取出来，放在/idcdata/xmltodb/vip2目录，给xmltodb_oracle入库到Oracle
/project/tools1/bin/procctl 3600 /project/tools1/bin/dminingmysql /log/idc/dminingmysql_ZHOBTMIND_vip2.log "<connstr>127.0.0.1,root,DYT.9525ing,TestDB,3306</connstr><charaset>utf8</charaset><selectsql>select obtid,date_format(ddatetime,'%%Y-%%m-%%d %%H:%%i:%%s'),t,p,u,wd,wf,r,vis,keyid from T_ZHOBTMIND where keyid>:1 and ddatetime>timestampadd(minute,-120,now())</selectsql><fieldstr>obtid,ddatetime,t,p,u,wd,wf,r,vis,keyid</fieldstr><fieldlen>10,19,8,8,8,8,8,8,8,15</fieldlen><bfilename>ZHOBTMIND</bfilename><efilename>HYCZ</efilename><outpath>/idcdata/xmltodb/vip2</outpath><incfield>keyid</incfield><timeout>30</timeout><pname>dminingmysql_ZHOBTMIND_vip2</pname><maxcount>1000</maxcount><connstr1>127.0.0.1,root,DYT.9525ing,TestDB,3306</connstr1>"

# 清理/idcdata/xmltodb/vip2 /idcdata/xmltodb/vip2bak 和 /idcdata/xmltodb/vip2err 目录中文件，防止把空间撑满
/project/tools1/bin/procctl 300 /project/tools1/bin/deletefiles /idcdata/xmltodb/vip2   "*" 0.04
/project/tools1/bin/procctl 300 /project/tools1/bin/deletefiles /idcdata/xmltodb/vip2bak "*" 0.04
/project/tools1/bin/procctl 300 /project/tools1/bin/deletefiles /idcdata/xmltodb/vip2err "*" 0.04