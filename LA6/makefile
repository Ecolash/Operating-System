
# Targets:
# - all: Compiles all programs.
# - db: Compiles and runs gencustomers, generating customers.txt.
# - clean: Removes all generated binaries and log files.
# - run: Launches cook, waiter, and customer in separate xterm windows.
# - store: Runs all programs and saves their output to respective transcript files.
# - bgrun: Runs all programs in the background and saves output to transcript_all.

# Usage:
# - Run `make all` to compile everything.
# - Run `make run` to start the simulation in separate terminals (requires xterm).
# - Run `make store` to log output to seperate transcript files.
# - Run `make bgrun` to run everything and generate a all transcript file.
# - Run `make clean` to remove generated files.

all:
	gcc -Wall -o cook cook.c
	gcc -Wall -o waiter waiter.c
	gcc -Wall -o customer customer.c
db:
	gcc -Wall -o gencustomers gencustomers.c
	./gencustomers > customers.txt

clean:
	rm -f cook waiter customer gencustomers transcript_cook.txt transcript_waiter.txt customer.txt 

run: all
	xterm -T "COOK" -fa "Monospace" -fs 12 -bg black -fg cyan -geometry 80x24+1000+0 -b 2 -e bash -c "./cook; exec bash" & sleep 1; \
	xterm -T "WAITER" -fa "Monospace" -fs 12 -bg black -fg cyan -geometry 80x24+0+0 -b 2 -e bash -c "./waiter; exec bash" & sleep 2; \
	xterm -T "CUSTOMER" -fa "Monospace" -fs 12 -bg black -fg cyan -geometry 80x24+0+550 -b 2 -e bash -c "./customer; exec bash" &

store: all
	./cook > transcript_cook.txt & 
	./waiter > transcript_waiter.txt &
	./customer > transcript_customer.txt

bgrun: all
	( ./cook & sleep 1; ./waiter & sleep 1; ./customer ) > transcript_all.txt 
 