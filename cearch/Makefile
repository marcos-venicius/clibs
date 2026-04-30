CC = gcc
CFLAGS = -Wall -Wextra -fPIC -g
LDFLAGS = -shared

SRC_DIR = src
BIN_DIR = bin

LIB_NAME = $(BIN_DIR)/libcearch.so
EXE_NAME = $(BIN_DIR)/cearch_test

SRCS = $(shell find $(SRC_DIR) -name '*.c')
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%.o, $(SRCS))

all: $(LIB_NAME) $(EXE_NAME)

$(LIB_NAME): $(OBJS) | $(BIN_DIR)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) -o $@ $^

$(EXE_NAME): $(OBJS) | $(BIN_DIR)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ $^

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c | $(BIN_DIR)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR):
	mkdir -p $@

clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean
