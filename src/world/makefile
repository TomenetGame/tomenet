CC=gcc
CFLAGS=-O2 -Wall -g

all: worldd

worldd: server.o world.o auth.o play.o lock.o account.o
	gcc -o worldd server.o world.o play.o lock.o account.o auth.o -lcrypt

clean:
	rm -f worldd *.o
