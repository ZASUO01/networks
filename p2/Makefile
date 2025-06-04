CC = gcc
CFLAGS = -Wall -Wextra -Iinclude 
LDFLAGS = -lm -g

BIN = bin
OBJ = obj
SRC = src
INCLUDE = include
LOG = logs

SRCS = $(wildcard $(SRC)/*.c)
OBJS = $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRCS))

TARGET = $(BIN)/main

all: $(TARGET) 

$(TARGET): $(OBJS) | $(BIN) $(LOG)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
		
$(OBJ)/%.o: $(SRC)/%.c | $(OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN) $(OBJ) $(LOG):
	mkdir -p $@

clean:
	rm -rf $(BIN) $(OBJ) $(LOG)
