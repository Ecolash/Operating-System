run: LRU.cpp
	g++ -Wall -o runsearch LRU.cpp
	./runsearch

vrun: LRU.cpp
	g++ -Wall -DVERBOSE -o runsearch LRU.cpp
	./runsearch

db: gensearch.c
	gcc -Wall -o gensearch gensearch.c
	./gensearch

clean:
	-rm -f runsearch gensearch