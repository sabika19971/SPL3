all: clean compile link

compile:
	g++ -g -Wall -Weffc++ -std=c++11 -Iinclude -c src/ConnectionHandler.cpp -o bin/ConnectionHandler.o
	g++ -g -Wall -Weffc++ -std=c++11 -Iinclude -c src/event.cpp -o bin/event.o
	g++ -g -Wall -Weffc++ -std=c++11 -Iinclude -c src/KeyBoardInput.cpp -o bin/KeyBoardInput.o
	g++ -g -Wall -Weffc++ -std=c++11 -Iinclude -c src/StompClient.cpp -o bin/StompClient.o

link:
	g++ -g -Wall -Weffc++ -std=c++11 -Iinclude bin/ConnectionHandler.o bin/event.o bin/KeyBoardInput.o bin/StompClient.o \
    -o bin/StompEMIClient -lpthread 

clean:
	rm -f bin/*.o bin/StompEMIClient	


























# CFLAGS:=-c -Wall -Weffc++ -g -std=c++11 -Iinclude
# LDFLAGS:=-lboost_system -lpthread
# # TODO change the makefile for StompEMIlient
# all: StompWCIClient

# EchoClient: bin/ConnectionHandler.o bin/echoClient.o
# 	g++ -o bin/EchoClient bin/ConnectionHandler.o bin/echoClient.o $(LDFLAGS)

# StompWCIClient: bin/ConnectionHandler.o bin/StompClient.o bin/event.o bin/KeyBoardInput.o
# 	g++ -o bin/StompWCIClient bin/ConnectionHandler.o bin/StompClient.o bin/event.o bin/KeyBoardInput.o $(LDFLAGS)

# bin/ConnectionHandler.o: src/ConnectionHandler.cpp
# 	g++ $(CFLAGS) -o bin/ConnectionHandler.o src/ConnectionHandler.cpp

# bin/echoClient.o: src/echoClient.cpp
# 	g++ $(CFLAGS) -o bin/echoClient.o src/echoClient.cpp

# bin/event.o: src/event.cpp
# 	g++ $(CFLAGS) -o bin/event.o src/event.cpp

# bin/StompClient.o: src/StompClient.cpp
# 	g++ $(CFLAGS) -o bin/StompClient.o src/StompClient.cpp

# bin/KeyBoardInput.o: src/KeyBoardInput.cpp
# 	g++ $(CFLAGS) -o bin/KeyBoardInput.o src/KeyBoardInput.cpp




# .PHONY: clean
# clean:
# 	rm -f bin/*

	