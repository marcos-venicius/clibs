CXX = gcc
CXX_FLAGS = -g -fsanitize=address -Wall -Wextra -pedantic

SRC = ll.c test-ll.c
OBJS = $(SRC:%.c=%.o)

ll: $(OBJS)
	$(CXX) $(CXX_FLAGS) -o $@ $^

%.o: %.c
	$(CXX) $(CXX_FLAGS) -c $< -o $@

clean:
	rm -rf ll $(OBJS)
