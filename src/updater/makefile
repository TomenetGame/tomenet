CC=gcc
CFLAGS=-Os -Wall -g

all: TomeNET-Updater.32b TomeNET-Updater.64b

TomeNET-Updater.32b: TomeNET-Updater.32b.o
	gcc $(CFLAGS) -m32 -o TomeNET-Updater.32b TomeNET-Updater.32b.o `pkg-config --libs gtk+-2.0`
	strip TomeNET-Updater.32b

TomeNET-Updater.32b.o: TomeNET-Updater.c
	gcc $(CFLAGS) -m32 -o TomeNET-Updater.32b.o -c TomeNET-Updater.c `pkg-config --cflags gtk+-2.0`

TomeNET-Updater.64b: TomeNET-Updater.64b.o
	gcc $(CFLAGS) -m64 -o TomeNET-Updater.64b TomeNET-Updater.64b.o `pkg-config --libs gtk+-2.0`
	strip TomeNET-Updater.64b

TomeNET-Updater.64b.o: TomeNET-Updater.c
	gcc $(CFLAGS) -m64 -o TomeNET-Updater.64b.o -c TomeNET-Updater.c `pkg-config --cflags gtk+-2.0`

clean:
	rm -f TomeNET-Updater.32b TomeNET-Updater.32b.o TomeNET-Updater.64b TomeNET-Updater.64b.o
