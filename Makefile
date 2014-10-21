CC = c99
LDLIBS = -lpthread

all: Server Client

Server: Server.o assign2lib.o

Client: Client.o

Server.o: Server.c

Client.o: Client.c

assign2lib.o: assign2lib.c

clean:
	rm -f *.o
