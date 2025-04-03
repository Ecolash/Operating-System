run: LRU.cpp
	g++ -Wall -o runsearch LRU.cpp
	./runsearch > output.txt
	@echo "Output written to output.txt"

vrun: LRU.cpp
	g++ -Wall -DVERBOSE -o runsearch LRU.cpp
	./runsearch > verbose.txt
	@echo "Verbose output written to verbose.txt"

db: gensearch.c
	gcc -Wall -o gensearch gensearch.c
	./gensearch

clean:
	-rm -f runsearch gensearch