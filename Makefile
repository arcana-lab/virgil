CPP=clang++
CFLAGS=-std=c++14 -g -I./include
LIBS=-pthreads
PROGRAMS=test1 test2 test3 baseline
OPT=-O3

all: $(PROGRAMS)

test1: test1.o
	$(CPP) $(LIBS) $(OPT) $^ -o $@

test2: test2.o
	$(CPP) $(LIBS) $(OPT) $^ -o $@

test3: test3.o
	$(CPP) $(LIBS) $(OPT) $^ -o $@

baseline: baseline.o
	$(CPP) $(LIBS) $(OPT) $^ -o $@

%.o: %.cpp
	$(CPP) $(CFLAGS) $(OPT) -c $^ -o $@

clean:
	rm -f *.o $(PROGRAMS)

.PHONY: clean
