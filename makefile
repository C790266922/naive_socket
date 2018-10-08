all: server client

server: server.c
	cc -g server.c -lpthread -o server
client: client.c
	cc -g client.c -lpthread -o client

.PHONY: clean
clean:
	rm -rf *.dSYM
	rm client server

