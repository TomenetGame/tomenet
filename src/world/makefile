CFLAGS=-g

all: worldd

worldd: server.o world.o auth.o play.o lock.o
	gcc -o worldd server.o world.o play.o lock.o auth.o -lcrypt

clean:
	rm worldd *.o
