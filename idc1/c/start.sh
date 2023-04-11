#############################
# 启动数据中心后台服务程序的脚本 #
#############################

# 检查服务程序是否超时，配置在/etc/rc.local中由root用户执行。
# /project/tools1/bin/procctl 30 /project/tools1/bin/checkproc

# 压缩数据中心后台服务程序的备份日志。
/project/tools1/bin/procctl 300 /project/tools1/bin/gzipfiles /log/idc "*.log.20*" 0.04

# 生成用于测试的全国气象站点观测的分钟数据到/tmp/idc/surfdata
/project/tools1/bin/procctl 60 /project/idc1/bin/crtsurfdata /project/idc1/ini/stcode.ini /tmp/idc/surfdata /log/idc/crtsurfdata.log xml,json,csv

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
/project/tools1/bin/procctl 10 /project/tools1/bin/fileserver /log/tools/fileserver1.log 5005

# 把目录/tmp/ftpputest中的文件上传到/tmp/tcpputest目录中
/project/tools1/bin/procctl 20 /project/tools1/bin/tcpputfiles /log/tools/tcpputfiles1.log "<ip>127.0.0.1</ip><port>5005</port><ptype>1</ptype><clientpath>/tmp/ftpputest</clientpath><serverpath>/tmp/tcpputest</serverpath><clientpathbak>/tmp/tcp/clientbak</clientpathbak><matchname>*.JSON</matchname><andchild>true</andchild><timetvl>10</timetvl><timeout>50</timeout><pname>tcpputfiles</pname>"

# 把目录/tmp/tcpputest中的文件下载到/tmp/tcpgetest目录中
/project/tools1/bin/procctl 20 /project/tools1/bin/tcpgetfiles /log/tools/tcpgetfiles.log "<ip>127.0.0.1</ip><port>5005</port><ptype>1</ptype><clientpath>/tmp/tcpgetest</clientpath><serverpath>/tmp/tcpputest</serverpath><serverpathbak>/tmp/tcp/serverbak</serverpathbak><matchname>*.JSON</matchname><andchild>true</andchild><timetvl>10</timetvl><timeout>50</timeout><pname>tcpgetfiles</pname>"

# 清理采集的全国气象站点观测的分钟数据目录/tmp/tcpgtest中的历史数据文件
/project/tools1/bin/procctl 300 /project/tools1/bin/deletefiles /tmp/tcpgetest "*" 0.04

# 将站点参数文件内容入库，如果站点内容不在库中，则插入；如果站点内容以修改，则更新
/project/tools1/bin/procctl 120 /project/idc1/bin/obtcodetodb /project/idc1/ini/stcode.ini "127.0.0.1,root,DYT.9525ing,TestDB,3306" utf8 /log/idc/obtcodetodb.log

# 把全国站点分钟观测数据保存到数据库的T_ZHOBTMIND表中，数据只插入，不更新
/project/tools1/bin/procctl 10 /project/idc1/bin/obtmindtodb /idcdata/surfdata "127.0.0.1,root,DYT.9525ing,TestDB,3306" utf8 /log/idc/obtmindtodb.log

# 清理T_ZHOBTMIND表中120分之前的数据，防止磁盘空间被撑满
/project/tools1/bin/procctl 120 /project/tools1/bin/execsql /project/idc1/sql/cleardata.sql "127.0.0.1,root,DYT.9525ing,TestDB,3306" utf8 /log/tools/execsql.log