CC=gcc 
CFLAGS=-Wall -Wextra -g -pthread
EXEC_EQUIPMENT=./equipment 
EXEC_SERVER=./server

all: $(EXEC_EQUIPMENT) $(EXEC_SERVER)

$(EXEC_EQUIPMENT): equipment.c common.o
	$(CC) $(CFLAGS) equipment.c common.o -o $(EXEC_EQUIPMENT)

$(EXEC_SERVER): server.c common.o
	$(CC) $(CFLAGS) server.c common.o -o $(EXEC_SERVER)

common.o: common.c
	$(CC) $(CFLAGS) -c common.c -o common.o

clean:
	rm -rf *.o server equipment