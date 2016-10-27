TARGET = OCEAN
OBJS = main.o jacobcalc.o laplacalc.o \
	slave1.o slave2.o multi.o
CFLAGS = -O2 -Olimit 2000 -w 
LDFLAGS = -lmpc -lm
MACROS = ../../../null_macros/c.m4.null

x = *

$(TARGET): $(OBJS)
	cc $(OBJS) -o $(TARGET) $(LDFLAGS)

.SUFFIXES:
.SUFFIXES:	.o .c .C .h .H

.H.h: 
	m4 ${MACROS} $*.H > $*.h

.C.c: 
	m4 $(MACROS) $*.C > $*.c

.c.o: 
	cc -c $(CFLAGS) $*.c

.C.o: 
	m4 $(MACROS) $*.C > $*.c
	cc -c $(CFLAGS) $*.c

decs.h: decs.H
jacobcalc.c: decs.h
main.c: decs.h
slave1.c: decs.h
laplacalc.c: decs.h
multi.c : decs.h
slave2.c: decs.h

