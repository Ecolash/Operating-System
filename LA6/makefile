all:
	gcc -Wall -o cook cook.c
	gcc -Wall -o waiter waiter.c
	gcc -Wall -o customer customer.c
db:
	gcc -Wall -o gencustomers gencustomers.c
	./gencustomers > customers.txt

clean:
	rm -f cook waiter customer gencustomers

run: all
	xterm -T "COOK" -fa "Monospace" -fs 12 -bg black -fg cyan -geometry 80x24+1000+0 -b 2 -e bash -c "./cook; exec bash" & \
	sleep 1; \
	xterm -T "WAITER" -fa "Monospace" -fs 12 -bg black -fg cyan -geometry 80x24+0+0 -b 2 -e bash -c "./waiter; exec bash" & \
	sleep 2; \
	xterm -T "CUSTOMER" -fa "Monospace" -fs 12 -bg black -fg cyan -geometry 80x24+0+550 -b 2 -e bash -c "./customer; exec bash" &