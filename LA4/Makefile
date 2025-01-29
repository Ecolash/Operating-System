all: boardgen.c block.c coordinator.c
	gcc -Wall -o block block.c
	gcc -Wall -o coordinator coordinator.c

run: all
	./coordinator

clean:
	-rm -f block coordinator