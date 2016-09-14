#
# Copyright 2016 - Phani Gadepalli
#

RM=rm
CC=gcc
CP=cp
CFLAGS=-g3 -O3 -I../include

all:
	$(CC) $(CFLAGS) -c kslab.c
	$(CP) kslab.o ../test/

clean:
	$(RM) -f *.o

distclean: clean
	$(RM) -f ../test/kslab.o
