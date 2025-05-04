# FILL THE PROGRAM VARIABLES HERE ------
ADDR4 = 
ADDR6 = 
PORT = 
LOCAL_ADDR4 = 
LOCAL_ADDR6 = 
LOCAL_PORT = 
GAS = 

# RULES --------------------------------
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
	rm -rf $@
	mkdir -p $@

md5-4: $(OUT)
	$(MD5) $(ADDR4):$(PORT) $(GAS) $(OUT)/md5_out.txt

md5-6: $(OUT)
	$(MD5) $(ADDR6):$(PORT) $(GAS) $(OUT)/md5_out.txt

xfer-c-4: $(OUT)
	$(XFER) -c $(LOCAL_ADDR4):$(LOCAL_PORT) client_input.txt $(OUT)/client_output.txt v4

xfer-s-4: $(OUT)
	$(XFER) -s $(LOCAL_PORT) server_input.txt $(OUT)/server_output.txt v4

xfer-c-6: $(OUT)
	$(XFER) -c $(LOCAL_ADDR6):$(LOCAL_PORT) client_input.txt $(OUT)/client_output.txt v6

xfer-s-6: $(OUT)
	$(XFER) -s $(LOCAL_PORT) server_input.txt $(OUT)/server_output.txt v6


test-md5-4: $(OUT)
	$(MD5) $(ADDR4):$(PORT) $(GAS) $(OUT)/md5_out.txt -d

test-md5-6: $(OUT)
	$(MD5) $(ADDR6):$(PORT) $(GAS) $(OUT)/md5_out.txt -d

test-xfer-s: $(OUT)
	$(XFER) -s $(LOCAL_PORT) server_input.txt $(OUT)/server_output.txt v6 -d 

test-xfer-c: $(OUT)
	$(XFER) -c $(LOCAL_ADDR6):$(LOCAL_PORT) client_input.txt $(OUT)/client_output.txt -d

clean:
	rm -rf $(BIN) $(OBJ) $(OUT)
