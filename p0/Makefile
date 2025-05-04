ADDR =150.164.213.243 
ADDR1 = 2804:1f4a:dcc:ff03::1
PORT = 51001 
COMMAND = gtr 1 2021039883:1:705e2ad17eb257f88670a5be2d5be59af2312c73aef08ed205ae8f1713b61b25

CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
LDFLAGS = -lm -g

BIN = bin
OBJ = obj
SRC = src
INCLUDE = include

SRCS = $(wildcard $(SRC)/*.c)
OBJS = $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRCS))

TARGET = $(BIN)/main

all: $(TARGET) 

$(TARGET): $(OBJS) | $(BIN)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
		
$(OBJ)/%.o: $(SRC)/%.c | $(OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN) $(OBJ):
	mkdir -p $@


mem-leak: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
	$(TARGET) $(ADDR) $(PORT) $(COMMAND)

test-ipv4:
	$(TARGET) $(ADDR) $(PORT) $(COMMAND) 

test-ipv6:
	$(TARGET) $(ADDR1) $(PORT) $(COMMAND)
	

clean:
	rm -rf $(BIN) $(OBJ)
