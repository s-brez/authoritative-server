.default: all

all: Shared

clean:
	rm -rf Shared *.o *.dSYM test

Shared: Shared.o
	g++ -Wall -Werror -std=c++14 -g -O -o $@ $^

%.o: %.cpp 
	g++ -Wall -std=c++14 -g -c $^
