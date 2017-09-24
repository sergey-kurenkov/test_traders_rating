.PHONY: all clean

all: traders_rating

traders_rating: src/traders_rating/service.o src/traders_rating/cmds.o src/main.o \
				src/traders_rating/utilities.o Makefile
	g++ src/main.o src/traders_rating/service.o src/traders_rating/cmds.o \
	src/traders_rating/utilities.o -o traders_rating

src/traders_rating/service.o: src/traders_rating/service.cpp include/traders_rating/service.h \
							  src/traders_rating/cmds.cpp include/traders_rating/cmds.h Makefile
	g++ -std=c++11 -I ./include -g -c src/traders_rating/service.cpp -o src/traders_rating/service.o


src/traders_rating/cmds.o: src/traders_rating/cmds.cpp include/traders_rating/cmds.h Makefile
	g++ -std=c++11 -I ./include -g -c src/traders_rating/cmds.cpp -o src/traders_rating/cmds.o

src/traders_rating/utilities.o: include/traders_rating/utilities.h src/traders_rating/utilities.cpp Makefile
	g++ -std=c++11 -I ./include -g -c src/traders_rating/utilities.cpp -o src/traders_rating/utilities.o

src/main.o: src/main.cpp Makefile
	g++ -std=c++11 -I ./include -g -c src/main.cpp -o src/main.o

clean:
	find . -type f -name "*.o" -exec rm {} \;
	find . -type f -name "traders_rating" -exec rm {} \;
