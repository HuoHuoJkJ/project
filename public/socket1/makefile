all: client01 server01 client11 server11 server06 client06 server20 server30 client30 tcpselect tcpclient tcppoll tcpepoll client03 server03

client01:client01.cpp
	g++ -g -o client01 client01.cpp ../_public.cpp -lm -lc

client06:client06.cpp
	g++ -g -o client06 client06.cpp ../_public.cpp -lm -lc

client30:client30.cpp
	g++ -g -o client30 client30.cpp ../_public.cpp -lm -lc

server01:server01.cpp
	g++ -g -o server01 server01.cpp ../_public.cpp -lm -lc

server06:server06.cpp
	g++ -g -o server06 server06.cpp ../_public.cpp -lm -lc

client11:client11.cpp
	g++ -g -o client11 client11.cpp ../_public.cpp -lm -lc

server11:server11.cpp
	g++ -g -o server11 server11.cpp ../_public.cpp -lm -lc

server20:server20.cpp
	g++ -g -o server20 server20.cpp ../_public.cpp -lm -lc -lpthread

server30:server30.cpp
	g++ -g -o server30 server30.cpp -I/project/public /project/public/_public.cpp -I/oracle/home/rdbms/public -I/project/public/db/oracle -L/oracle/home/lib -L. -lclntsh /project/public/db/oracle/_ooci.cpp -lm -lc

tcpclient:tcpclient.cpp
	g++ -o tcpclient tcpclient.cpp

tcpselect:tcpselect.cpp
	g++ -o tcpselect tcpselect.cpp

tcppoll:tcppoll.cpp
	g++ -o tcppoll tcppoll.cpp

tcpepoll:tcpepoll.cpp
	g++ -o tcpepoll tcpepoll.cpp

client03:client03.cpp
	g++ -o client03 client03.cpp

server03:server03.cpp
	g++ -o server03 server03.cpp

clean:
	rm client01 server01 client11 server11 server06 client06 server20 server30 client30 tcpselect tcpclient tcppoll tcpepoll client03 server03