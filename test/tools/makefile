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

# 编译参数
CFLAGS = -g
# procctl
all: checkpro hello

checkpro:checkpro.cpp
	g++ $(CFLAGS) -o checkpro checkpro.cpp $(PUBINCL) $(PUBCPP) -lm -lc

hello:hello.cpp
	g++ $(CFLAGS) -o hello hello.cpp $(PUBINCL) $(PUBCPP) -lm -lc

clean:
	rm -f checkpro hello