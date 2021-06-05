.default: all

all: Client

clean:
	rm -rf Client *.o *.dSYM test

Client: Client.o Shared.o
	g++ -Wall -Werror -std=c++14 -g -O -o $@ $^

%.o: %.cpp
	g++ -Wall -std=c++14 -g -c $^
