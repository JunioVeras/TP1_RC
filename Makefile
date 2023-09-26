CC = gcc
SRC = src
OBJ = obj
INC = include
BIN = bin
OBJS = $(OBJ)/common.o  
HDRS = $(INC)/common.h
CFLAGS = -Wall -c -I $(INC) -g

EXE1 = $(BIN)/server
EXE2 = $(BIN)/client

all: $(EXE1) $(EXE2)

$(EXE1): $(OBJS) $(OBJ)/server.o
	$(CC) -o $(BIN)/server $(OBJS) $(OBJ)/server.o

$(EXE2): $(OBJS) $(OBJ)/client.o
	$(CC) -o $(BIN)/client $(OBJS) $(OBJ)/client.o

$(OBJ)/client.o: $(HDRS) $(SRC)/client.c
	$(CC) $(CFLAGS) -o $(OBJ)/client.o $(SRC)/client.c

$(OBJ)/server.o: $(HDRS) $(SRC)/server.c
	$(CC) $(CFLAGS) -o $(OBJ)/server.o $(SRC)/server.c

$(OBJ)/common.o: $(HDRS) $(SRC)/common.c
	$(CC) $(CFLAGS) -o $(OBJ)/common.o $(SRC)/common.c

clean:
	rm $(OBJ)/*.o
	rm $(BIN)/*