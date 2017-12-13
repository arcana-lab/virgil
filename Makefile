CPP=clang++
OBJS=test.o
CFLAGS=-std=c++14

all: test

test: $(OBJS)
	$(CPP) $^ -o $@

%.o: %.cpp
	$(CPP) $(CFLAGS) -c $^ -o $@

clean:
	rm -f *.o test

.PHONY: clean
