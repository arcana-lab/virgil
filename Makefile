CPP=clang++
OBJS=test.o
CFLAGS=-std=c++14
LIBS=-pthreads

all: test

test: $(OBJS)
	$(CPP) $(LIBS) $^ -o $@

%.o: %.cpp
	$(CPP) $(CFLAGS) -c $^ -o $@

clean:
	rm -f *.o test

.PHONY: clean
