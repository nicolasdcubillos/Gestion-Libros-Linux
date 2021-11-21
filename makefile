all: receptor solicitante

solicitante: solicitante.o libproyecto.h
	gcc -o solicitante solicitante.o

solicitante.o: solicitante.c libproyecto.h
	gcc -c solicitante.c

receptor: receptor.o libproyecto.h
	gcc -o receptor receptor.o -pthread

receptor.o: receptor.c libproyecto.h
	gcc -c receptor.c
