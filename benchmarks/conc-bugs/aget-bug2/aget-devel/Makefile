# EnderUNIX Aget Makefile
# http://www.enderunix.org/aget/

OBJS = main.o Aget.o Misc.o Head.o Signal.o Download.o Ftp.o Resume.o loadrc.o
CFLAGS = -g -Wall 
LDFLAGS = -pthread
CC = gcc
STRIP = strip




all: aget

aget: $(OBJS)
	$(CC) -o aget -g $(OBJS) $(LDFLAGS)

install: aget
	cp -f aget /usr/local/bin/aget

clean:
	rm -f *.o aget
