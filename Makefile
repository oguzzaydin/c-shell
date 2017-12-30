




CC = gcc
CFLAGS = -Wall -g
OBJ = kabuk.o

all: derle

derle: $(OBJ)
	$(CC) $(CFLAGS) -o kabuk $(OBJ) 

%.o: %.c
	$(CC) $(CFLAGS) -c $<
