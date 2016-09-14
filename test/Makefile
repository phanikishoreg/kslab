RM=rm
CC=gcc
CP=cp
CFLAGS=-g3 -O3 -I../include

all:
	$(CC) $(CFLAGS) -c kslab_test.c
	$(CC) $(CFLAGS) *.o -o kslab_test

clean:
	$(RM) -f *.o kslab_test
