CC=gcc
CC_OPT=-Wall -std=gnu9x -c -g -O2
DEPS=main.o heap.o
PRGM=test

MAIN: $(DEPS)
	$(CC) $(DEPS) -o $(PRGM)	

%.o : %.c
	$(CC) $(CC_OPT) $<

clean:
	rm *.o $(PRGM)

