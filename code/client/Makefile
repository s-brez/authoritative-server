.default: all

all: Client

clean:
	rm -rf Client *.o *.dSYM test

Client: Client.o ../shared/Shared.o -lssl -lcrypto 
	g++ -Wall -Werror -lssl -lcrypto -std=c++14 -g -O -o $@ $^

%.o: %.cpp
	g++ -Wall -std=c++14 -g -c $^
