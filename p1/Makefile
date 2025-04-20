ADDR = 150.164.213.243
ADDR1 = 2804:1f4a:dcc:ff03::1
PORT = 51510
GAS = 2019035400:6:81f9721a4971e9b8ddf23847e3a374eda2b25c245e126ed058a4b17d7b852209+fcafd0e13cc284dea86db12d1bf37540b55efb0eab9d0ab279d80c9558cc1368
 
CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
LDFLAGS = -lm -lssl -lcrypto -g

SRC = src
BIN = bin
OBJ = obj
OUT = output

ALL_SRCS = $(wildcard $(SRC)/*.c)
MAIN_SRCS = $(SRC)/dccnet-md5.c $(SRC)/dccnet-xfer.c
AUX_SRCS = $(filter-out $(MAIN_SRCS), $(ALL_SRCS))
AUX_OBJS = $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(AUX_SRCS))

MD5 = $(BIN)/dccnet-md5
XFER = $(BIN)/dccnet-xfer

all: $(MD5) $(XFER)

$(MD5): $(OBJ)/dccnet-md5.o $(AUX_OBJS) | $(BIN)
	$(CC) $(LDFLAGS) $^ -o $@

$(XFER): $(OBJ)/dccnet-xfer.o $(AUX_OBJS) | $(BIN)
	$(CC) $(LDFLAGS) $^ -o $@

$(OBJ)/%.o: $(SRC)/%.c | $(OBJ)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN) $(OBJ) $(OUT):
	mkdir -p $@

test-md5: $(OUT) 
	$(MD5) $(ADDR) $(PORT) $(GAS) -d

mem-leak-md5: $(MD5)
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
	$(MD5) $(ADDR) $(PORT) $(GAS)


clean:
	rm -rf $(BIN) $(OBJ) $(OUT)
