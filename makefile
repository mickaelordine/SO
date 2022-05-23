CC=gcc
CFLAGS=-std=c89 -pedantic
master: master.c util.o utente.o
	${CC} ${CFLAGS} master.c util.o utente.o -o master
utente.o: utente.c
	${CC} ${CFLAGS} utente.c -c
util.o: util.c
	${CC} ${CFLAGS} util.c -c
clean :
	-rm utente.o util.o