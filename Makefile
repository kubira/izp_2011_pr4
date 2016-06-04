program=proj4

SRC=proj4.c

CC=gcc

CFLAGS = -std=c99 -Wall -Wextra -pedantic -g -lm

${program}:
	${CC} ${SRC} -o ${program} ${CFLAGS}

clean:
	rm -f ${program}
