INC = -I./headers
all: server client

server: server.o commandFunctions.o helperFunctions.o
	g++ -std=c++17 -Wall -Werror -o server $^

client: client.o clientRequests.o
	g++ -std=c++17 -Wall -Werror -o client $^

%.o: %.cpp
	g++ -std=c++17 -Wall -Werror $(INC) -c -o $@ $<

clean:
	rm -f server client *.o