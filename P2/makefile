scanner puzzle: scanner_copy.o puzzle.o
	g++ -Wall scanner_copy.o -o scanner
	g++ -Wall puzzle.o -o puzzle

scanner.o puzzle.o: scanner.cpp puzzle.o
	g++ -c -std=c++11 scanner_copy.cpp
	g++ -c -std=c++11 puzzle.cpp

clean:
	rm *.o
