.default: all

all: Server

clean:
	rm -rf Server *.o *.dSYM test

Server: Server.o ../shared/Shared.o -lssl -lcrypto 
	g++ -Wall -Werror -lssl -lcrypto -std=c++14 -g -O -o $@ $^

%.o: %.cpp 
	g++ -Wall -std=c++14 -g -c $^
