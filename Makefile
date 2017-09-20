.PHONY: all clean

all: traders_rating

traders_rating: src/traders_rating/service.o src/main.o Makefile
	g++ src/main.o src/traders_rating/service.o -o traders_rating

src/traders_rating/service.o: src/traders_rating/service.cpp src/traders_rating/service.h Makefile
	g++ -std=c++11 -I ./src -g -c src/traders_rating/service.cpp -o src/traders_rating/service.o

src/main.o: src/main.cpp Makefile
	g++ -std=c++11 -I ./src -g -c src/main.cpp -o src/main.o

clean:
	find . -type f -name "*.o" -exec rm {} \;
	find . -type f -name "traders_rating" -exec rm {} \;
