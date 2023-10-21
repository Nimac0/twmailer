all: server client

server: server.cpp
	g++ -std=c++17 -Wall -Werror -o server server.cpp commandFunctions.h helperFunctions.h

client: client.cpp
	g++ -std=c++17 -Wall -Werror -o client client.cpp clientRequests.h

clean:
	rm -f server client
