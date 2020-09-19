scanner puzzle: scanner.o puzzle.o
	g++ scanner.o -o scanner
	g++ puzzle.o -o puzzle

scanner.o puzzle.o: scanner.cpp puzzle.cpp
	g++ -c -Wall -std=c++11 scanner.cpp
	g++ -c -Wall -std=c++11 scanner.cpp

clean:
	rm *.o
