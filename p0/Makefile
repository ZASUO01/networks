PORT = 
ADDR =  
ADDR1 = 
COMMAND = 

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
