all: client01 server01

client01:client01.cpp
	g++ -g -o client01 client01.cpp ../_public.cpp -lm -lc

server01:server01.cpp
	g++ -g -o server01 server01.cpp ../_public.cpp -lm -lc
	
client11:client11.cpp
	g++ -g -o client11 client11.cpp ../_public.cpp -lm -lc

server11:server11.cpp
	g++ -g -o server11 server11.cpp ../_public.cpp -lm -lc

clean:
	rm client01 server01