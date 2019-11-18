DEBUG = 0

DFLAG = 

ifeq ($(DEBUG), 1)
	DFLAG = -DDEBUG
else
	DFLAG = 
endif


virtmem: main.o page_table.o disk.o program.o test.o
	gcc $(DFLAG) main.o page_table.o disk.o program.o test.o -o virtmem

main.o: main.c
	gcc $(DFLAG)  -Wall -g -c main.c -o main.o

page_table.o: page_table.c
	gcc $(DFLAG) -Wall -g -c page_table.c -o page_table.o

disk.o: disk.c
	gcc $(DFLAG) -Wall -g -c disk.c -o disk.o

program.o: program.c
	gcc $(DFLAG) -Wall -g -c program.c -o program.o

test.o: test.c
	gcc -Wall -g -c test.c -o test.o

clean:
	rm -f *.o virtmem
