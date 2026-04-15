CC = gcc
CFLAGS = -Wall -Wextra -fPIC -g
LDFLAGS = -shared

# Directories
SRC_DIR = src
BIN_DIR = bin

# Output files
LIB_NAME = $(BIN_DIR)/libcearch.so
EXE_NAME = $(BIN_DIR)/cearch_test

# Find all .c files in src/
SRCS = $(wildcard $(SRC_DIR)/*.c)

# Translate src/filename.c to bin/filename.o
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%.o, $(SRCS))

# Default target
all: $(LIB_NAME) $(EXE_NAME)

# Rule to create the Shared Library
$(LIB_NAME): $(OBJS) | $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $^

# Rule to create the test Executable
$(EXE_NAME): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

# Pattern Rule: Compile .c into .o and put it in bin/
$(BIN_DIR)/%.o: $(SRC_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to create the bin directory
$(BIN_DIR):
	mkdir -p $@

clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean
