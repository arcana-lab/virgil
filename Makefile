CPP=clang++
CFLAGS=-std=c++14
LIBS=-pthreads

all: test1 test2

test1: test1.o
	$(CPP) $(LIBS) $^ -o $@

test2: test2.o
	$(CPP) $(LIBS) $^ -o $@

%.o: %.cpp
	$(CPP) $(CFLAGS) -c $^ -o $@

clean:
	rm -f *.o test1 test2

.PHONY: clean
