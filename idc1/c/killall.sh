#############################
# 终止数据中心后台服务程序的脚本 #
#############################
killall -9 procctl
killall gzipfiles deletefiles crtsurfdata ftpgetfiles ftpgetfiles obtcodetodb execsql

sleep 3

killall -9 gzipfiles deletefiles crtsurfdata ftpgetfiles ftpgetfiles obtcodetodb execsql