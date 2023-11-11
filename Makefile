CC = g++
CFLAGS = -std=c++17 -Wall -Werror
LIBS = -lldap -llber
INC = -I./headers
all: server client

server: server.o commandFunctions.o helperFunctions.o ldapClient.o
	${CC} ${CFLAGS} -o server $^ ${LIBS}

client: client.o mypw.o clientRequests.o
	${CC} ${CFLAGS} -o client $^

%.o: %.cpp
	${CC} ${CFLAGS} $(INC) -c -o $@ $<

clean:
	rm -f server client *.o