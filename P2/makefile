scanner puzzle: scanner.o puzzle.o
	g++ -Wall scanner.o -o scanner
	g++ -Wall puzzle.o -o puzzle

scanner.o puzzle.o: scanner.cpp puzzle.o
	g++ -c -std=c++11 scanner.cpp
	g++ -c -std=c++11 puzzle.cpp

clean:
	rm *.o
