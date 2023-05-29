all: client01 server01 client11 server11 server06 client06 server20

client01:client01.cpp
	g++ -g -o client01 client01.cpp ../_public.cpp -lm -lc

client06:client06.cpp
	g++ -g -o client06 client06.cpp ../_public.cpp -lm -lc

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

clean:
	rm client01 server01 client11 server11 server06 client06 server20