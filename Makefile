CPP=clang++
CFLAGS=-std=c++14 -g
LIBS=-pthreads
PROGRAMS=test1 test2
OPT=-O3

all: $(PROGRAMS)

test1: test1.o
	$(CPP) $(LIBS) $(OPT) $^ -o $@

test2: test2.o
	$(CPP) $(LIBS) $(OPT) $^ -o $@

%.o: %.cpp
	$(CPP) $(CFLAGS) $(OPT) -c $^ -o $@

clean:
	rm -f *.o $(PROGRAMS)

.PHONY: clean
