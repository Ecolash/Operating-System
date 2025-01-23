compile: schedule.c
	gcc -Wall -o schedule schedule.c
	
run: compile db
	./schedule
	
vcompile: schedule.c
	gcc -Wall -o schedule -DVERBOSE schedule.c
	
vrun: vcompile
	./schedule
	
db: genproc.c
	gcc -Wall -o genproc genproc.c
	./genproc 20
	
clean:
	-rm -f genproc schedule proc.txt
