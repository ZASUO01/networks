ADDR = 150.164.213.243
ADDR1 = 2804:1f4a:dcc:ff03::1
PORT = 51510
GAS =2019035400:1:7ba4c632bce758b75134165779a6ee63de8d42e10ccf93bb85c3e4584a18c41f+dfe310b8ff7218f2075c24f3f1d994afac96aa14c3e0098bf5f310d9f537a5a4  

CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
LDFLAGS = -lm -g

BIN = bin
SRC = src
OBJ = obj

SRCS = $(wildcard $(SRC)/*.c)
OBJS = $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRCS))

TARGET = $(BIN)/main

$(TARGET): $(OBJS) | $(BIN)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ)/%.o: $(SRC)/%.c | $(OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN) $(OBJ):
	mkdir -p $@

test:
	$(TARGET) $(ADDR) $(PORT) $(GAS)

clean:
	rm -rf $(BIN) $(OBJ)
