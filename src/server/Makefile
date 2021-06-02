.default: all

all: Server

clean:
	rm -rf Server *.o *.dSYM test

Server: Server.o
	g++ -Wall -Werror -std=c++14 -g -O -o $@ $^

%.o: %.cpp
	g++ -Wall -std=c++14 -g -c $^
