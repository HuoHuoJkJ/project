# 开发框架头文件路径。
PUBINCL = -I/project/public

# 开发框架cpp文件名，这里直接包含进来，没有采用链接库，是为了方便调试。
PUBCPP = /project/public/_public.cpp

# mysql头文件存放的目录。
MYSQLINCL = -I/usr/include/mysql -I/project/public/db/mysql

# mysql库文件存放的目录。
MYSQLLIB = -L/usr/lib64/mysql/

# 需要链接的mysql库。
MYSQLLIBS = -lmysqlclient

# 开发框架mysql的cpp文件，这里直接包含进来，没有采用链接库，是为了方便调试
MYSQLCPP = /project/public/db/mysql/_mysql.cpp

# oracle 头文件路径
ORAINCL = -I$(ORACLE_HOME)/rdbms/public -I/project/public/db/oracle

# oracle 库文件路径
ORALIB = -L$(ORACLE_HOME)/lib -L.

# oracle 的 oci 库
ORALIBS = -lclntsh

# 开发框架mysql的cpp文件名，这里直接包含进来，没有采用链接库，是为了方便调试
ORACPP = /project/public/db/oracle/_ooci.cpp

# 编译参数
CFLAGS = -g

all: connectdb pthreadcreate

connectdb:connectdb.cpp
	g++ $(CFLAGS) -o connectdb connectdb.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORACPP) $(ORALIB) $(ORALIBS) -lm -lc

pthreadcreate:pthreadcreate.cpp
	g++ $(CFLAGS) -o pthreadcreate pthreadcreate.cpp $(PUBINCL) $(PUBCPP) -lpthread -lm -lc

checkproc:checkproc.cpp
	g++ $(CFLAGS) -o checkproc checkproc.cpp $(PUBINCL) $(PUBCPP) -lm -lc
	cp checkproc ../bin/.

ftpgetfiles:ftpgetfiles.cpp
	g++ $(CFLAGS) -o ftpgetfiles ftpgetfiles.cpp $(PUBINCL) $(PUBCPP) /project/public/libftp.a /project/public/_ftp.cpp -lm -lc
	cp ftpgetfiles ../bin/.

ftpputfiles:ftpputfiles.cpp
	g++ $(CFLAGS) -o ftpputfiles ftpputfiles.cpp $(PUBINCL) $(PUBCPP) /project/public/libftp.a /project/public/_ftp.cpp -lm -lc
	cp ftpputfiles ../bin/.

tcpputfiles:tcpputfiles.cpp
	g++ $(CFLAGS) -o tcpputfiles tcpputfiles.cpp $(PUBINCL) $(PUBCPP) -lm -lc
	cp tcpputfiles ../bin/.

fileserver:fileserver.cpp
	g++ $(CFLAGS) -o fileserver fileserver.cpp $(PUBINCL) $(PUBCPP) -lm -lc
	cp fileserver ../bin/.

execsql:execsql.cpp
	g++ $(CFLAGS) -o execsql execsql.cpp $(PUBINCL) $(PUBCPP) $(MYSQLINCL) $(MYSQLCPP) $(MYSQLLIB) $(MYSQLLIBS) -lm -lc
	cp execsql ../bin/.

dminingmysql:dminingmysql.cpp
	g++ $(CFLAGS) -o dminingmysql dminingmysql.cpp $(PUBINCL) $(PUBCPP) $(MYSQLINCL) $(MYSQLCPP) $(MYSQLLIB) $(MYSQLLIBS) -lm -lc
	cp dminingmysql ../bin/.

dminingoracle:dminingoracle.cpp
	g++ $(CFLAGS) -o dminingoracle dminingoracle.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORACPP) $(ORALIB) $(ORALIBS) -lm -lc
	cp dminingoracle ../bin/.

xmltodb:xmltodb.cpp _tools.h _tools.cpp
	g++ $(CFLAGS) -o xmltodb xmltodb.cpp _tools.cpp $(PUBINCL) $(PUBCPP) $(MYSQLINCL) $(MYSQLCPP) $(MYSQLLIB) $(MYSQLLIBS) -lm -lc
	cp xmltodb ../bin/.

xmltodb_oracle:xmltodb_oracle.cpp _tools_oracle.h _tools_oracle.cpp
	g++ $(CFLAGS) -o xmltodb_oracle xmltodb_oracle.cpp _tools_oracle.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORACPP) $(ORALIB) $(ORALIBS) -lm -lc
	cp xmltodb_oracle ../bin/.

syncupdate:syncupdate.cpp _tools.h _tools.cpp
	g++ $(CFLAGS) -o syncupdate syncupdate.cpp _tools.cpp $(PUBINCL) $(PUBCPP) $(MYSQLINCL) $(MYSQLCPP) $(MYSQLLIB) $(MYSQLLIBS) -lm -lc
	cp syncupdate ../bin/.

syncupdate_oracle:syncupdate_oracle.cpp _tools_oracle.h _tools_oracle.cpp
	g++ $(CFLAGS) -o syncupdate_oracle syncupdate_oracle.cpp _tools_oracle.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORACPP) $(ORALIB) $(ORALIBS) -lm -lc
	cp syncupdate_oracle ../bin/.

syncincrement:syncincrement.cpp _tools.h _tools.cpp
	g++ $(CFLAGS) -o syncincrement syncincrement.cpp _tools.cpp $(PUBINCL) $(PUBCPP) $(MYSQLINCL) $(MYSQLCPP) $(MYSQLLIB) $(MYSQLLIBS) -lm -lc
	cp syncincrement ../bin/.

syncincrement_oracle:syncincrement_oracle.cpp _tools_oracle.h _tools_oracle.cpp
	g++ $(CFLAGS) -o syncincrement_oracle syncincrement_oracle.cpp _tools_oracle.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORACPP) $(ORALIB) $(ORALIBS) -lm -lc
	cp syncincrement_oracle ../bin/.

syncincrementex:syncincrementex.cpp _tools.h _tools.cpp
	g++ $(CFLAGS) -o syncincrementex syncincrementex.cpp _tools.cpp $(PUBINCL) $(PUBCPP) $(MYSQLINCL) $(MYSQLCPP) $(MYSQLLIB) $(MYSQLLIBS) -lm -lc
	cp syncincrementex ../bin/.

syncincrementex_oracle:syncincrementex_oracle.cpp _tools_oracle.h _tools_oracle.cpp
	g++ $(CFLAGS) -o syncincrementex_oracle syncincrementex_oracle.cpp _tools_oracle.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORACPP) $(ORALIB) $(ORALIBS) -lm -lc
	cp syncincrementex_oracle ../bin/.

deletetabledata:deletetabledata.cpp
	g++ $(CFLAGS) -o deletetabledata deletetabledata.cpp $(PUBINCL) $(PUBCPP) $(MYSQLINCL) $(MYSQLCPP) $(MYSQLLIB) $(MYSQLLIBS) -lm -lc
	cp deletetabledata ../bin/.

deletetabledata_oracle:deletetabledata_oracle.cpp
	g++ $(CFLAGS) -o deletetabledata_oracle deletetabledata_oracle.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORACPP) $(ORALIB) $(ORALIBS) -lm -lc
	cp deletetabledata_oracle ../bin/.

migratetabledata:migratetabledata.cpp _tools.h _tools.cpp
	g++ $(CFLAGS) -o migratetabledata migratetabledata.cpp _tools.cpp $(PUBINCL) $(PUBCPP) $(MYSQLINCL) $(MYSQLCPP) $(MYSQLLIB) $(MYSQLLIBS) -lm -lc
	cp migratetabledata ../bin/.

migratetabledata_oracle:migratetabledata_oracle.cpp _tools_oracle.h _tools_oracle.cpp
	g++ $(CFLAGS) -o migratetabledata_oracle migratetabledata_oracle.cpp _tools_oracle.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORACPP) $(ORALIB) $(ORALIBS) -lm -lc
	cp migratetabledata_oracle ../bin/.

webserver:webserver.cpp
	g++ $(CFLAGS) -o webserver webserver.cpp $(PUBINCL) $(PUBCPP) $(ORAINCL) $(ORACPP) $(ORALIB) $(ORALIBS) -lpthread -lm -lc
	cp webserver ../bin/.

clean:
	rm -f connectdb