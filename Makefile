CC=gcc
CFLAGS=-g -Wall -W
LDFLAGS=-lz 
SRC=packet_implem.c
OBJ=$(SRC:.c=.0)

all : packet

packet : packet.o
	gcc -o packet packet.o $(LDFLAGS)

packet.o : packet_implem.c 
	gcc -o packet.o -c packet_implem.c $(CFLAGS) 