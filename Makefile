cc=gcc
flags=-Wall
clientLibraries=-pthread

all: client server

client: client.c chat.h
	$(cc) $(flags) $(clientLibraries) client.c -o client

server: server.c chat.h
	$(cc) $(flags) server.c -o server

clean:
	rm client server
