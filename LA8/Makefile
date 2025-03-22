all:
	gcc -Wall -o resource -pthread resource.c
	gcc -Wall -D_DLAVOID -o resource_nodeadlock -pthread resource.c

allow: all
	./resource

avoid: all
	./resource_nodeadlock

db: geninput.c
	gcc -Wall -o geninput geninput.c
	./geninput 10 20

clean:
	-rm -f resource resource_nodeadlock geninput