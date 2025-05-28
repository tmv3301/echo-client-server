all: echo-server echo-client

echo-server: echo-server.cpp
	g++ -Wall -pthread -o echo-server echo-server.cpp

echo-client: echo-client.cpp
	g++ -Wall -pthread -o echo-client echo-client.cpp

clean:
	rm -f echo-server echo-client

