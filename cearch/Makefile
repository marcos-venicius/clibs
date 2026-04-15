CC = gcc
CFLAGS = -Wall -Wextra -fPIC -g
LDFLAGS = -shared

LIB_NAME = libcearch.so
EXE_NAME = cearch_test

SRCS = $(wildcard src/*.c)
OBJS = $(SRCS:.c=.o)

all: $(LIB_NAME) $(EXE_NAME)

$(LIB_NAME): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

$(EXE_NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(LIB_NAME) $(EXE_NAME)
