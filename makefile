
OBJECTS=fat.o minix.o

fat2minix: fat2minix.c fat2minix.h ${OBJECTS}
	cc -Wall -o fat2minix fat2minix.c ${OBJECTS}

fat.o: fat.h fat.c
	cc -Wall -c -o fat.o fat.c

minix.o: minix.h minix.c
	cc -Wall -c -o minix.o minix.c
