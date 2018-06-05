CPP=clang++
CFLAGS=-std=c++14 -g -I../../include
LIBS=-pthreads
PROFILER=/usr/bin/time taskset -c 1
PROFILER_PERF=perf record --call-graph fp -e cpu-clock taskset -c 1
PROFILER_SHOW=perf report --stdio
OPT=-O3
INPUTS=100000000

all: work

work: work.o
	$(CPP) $(LIBS) $(OPT) $^ -o $@

%.o: %.cpp
	$(CPP) $(CFLAGS) $(OPT) -c $^ -o $@

run: work
	$(PROFILER)  ./$^ $(INPUTS)

clean:
	rm -f *.o work perf.*

.PHONY: clean run
