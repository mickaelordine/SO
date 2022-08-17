CC=gcc
CFLAGS=-std=c89 -pedantic
master: newmaster.c user node functions.c
	${CC} ${CFLAGS} functions.c newmaster.c -lm
	./a.out
	ipcs
	ps
user: newuser.c functions.c
	${CC} ${CFLAGS} -o user functions.c newuser.c
node: newnode.c functions.c nodeReader
	${CC} ${CFLAGS} -o node functions.c newnode.c
nodeReader: newnodereader.c functions.c
	${CC} ${CFLAGS} -o nodeReader functions.c newnodereader.c

size: lmsize.c 
	${CC} ${CFLAGS} lmsize.c
	./a.out

clean :
	-rm node user		#non so come funga
