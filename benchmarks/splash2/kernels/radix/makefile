TARGET = RADIX
OBJS = radix.o
CFLAGS = -O2 -Olimit 2000 -w 
LDFLAGS = -lmpc -lm
MACROS = ../../null_macros/c.m4.null

x = *

$(TARGET): $(OBJS)
	cc $(OBJS) $(CFLAGS) -o $(TARGET) $(LDFLAGS)

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

