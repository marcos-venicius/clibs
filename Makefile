CXX = gcc
CXX_FLAGS = -g -fsanitize=address -Wall -Wextra -pedantic

LL_TEST_SRC = ll.c test-ll.c
LL_TEST_OBJS = $(LL_TEST_SRC:%.c=%.o)

DEMO_SRC = ll.c demo.c
DEMO_OBJS = $(DEMO_SRC:%.c=%.o)

ll: $(LL_TEST_OBJS)
	$(CXX) $(CXX_FLAGS) -o $@ $^

demo: $(DEMO_OBJS)
	$(CXX) $(CXX_FLAGS) -o $@ $^

ll_lib:
	$(CXX) -c ll.c -fPIC -o ll.o
	$(CXX) ll.o -shared -o libll.so
	mkdir -p build/ll/include
	mv libll.so build/ll
	cp ll.h build/ll/include

%.o: %.c
	$(CXX) $(CXX_FLAGS) -c $< -o $@

clean:
	rm -rf demo build *.o *.so ll $(LL_TEST_OBJS) $(DEMO_OBJS)
