CFLAGS=-g

all: worldd

worldd: server.o world.o auth.o
	gcc -o worldd server.o world.o auth.o -lcrypt

clean:
	rm worldd *.o
