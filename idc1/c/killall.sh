#############################
# 终止数据中心后台服务程序的脚本 #
#############################
killall -9 procctl
killall gzipfiles deletefiles crtsurfdata ftpgetfiles ftpgetfiles tcpputfiles tcpgetfiles fileserver obtcodetodb execsql dminingmysql xmltodb syncupdate syncincrement deletetabledata migratetabledata xmltodb_oracle migratetabledata_oracle deletetabledata_oracle

sleep 3

killall -9 gzipfiles deletefiles crtsurfdata ftpgetfiles ftpgetfiles tcpputfiles tcpgetfiles fileserver obtcodetodb execsql dminingmysql xmltodb syncupdate syncincrement deletetabledata migratetabledata xmltodb_oracle migratetabledata_oracle deletetabledata_oracle