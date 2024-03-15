all: integration

integration: main.o
	gcc -fopenmp -lm main.o -o integration

main.o: main.c
	gcc -fopenmp -lm -c main.c

clean:
	rm -f *.o integration