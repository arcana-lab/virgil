CPP=clang++
CFLAGS=-std=c++14 -g -I../../include
LIBS=-pthreads
PROFILER=/usr/bin/time taskset -c 2,4
PROFILER_PERF=perf record --call-graph fp -e cpu-clock taskset -c 1
PROFILER_SHOW=perf report --stdio
OPT=-O3
INPUTS=100000000

all: work1 work2

work1: work1.o
	$(CPP) $(LIBS) $(OPT) $^ -o $@

work2: work2.o
	$(CPP) $(LIBS) $(OPT) $^ -o $@

%.o: %.cpp
	$(CPP) $(CFLAGS) $(OPT) -c $^ -o $@

run1: work1
	$(PROFILER)  ./$^ $(INPUTS)

run2: work2
	$(PROFILER)  ./$^ $(INPUTS)

clean:
	rm -f *.o work1 work2 perf.*

.PHONY: clean run
