CPP=clang++
CFLAGS=-std=c++14 -g -I../../include
LIBS=-pthreads
PROFILER=/usr/bin/time taskset -c 2,4
PROFILER_PERF=perf record --call-graph fp -e cpu-clock taskset -c 1
PROFILER_SHOW=perf report --stdio
OPT=-O3
INPUTS=100000000
PROGRAMS=work1 work2 work3 work4

all: $(PROGRAMS)

work1: work1.o
	$(CPP) $(LIBS) $(OPT) $^ -o $@

work2: work2.o
	$(CPP) $(LIBS) $(OPT) $^ -o $@

work3: work3.o
	$(CPP) $(LIBS) $(OPT) $^ -o $@

work4: work4.o
	$(CPP) $(LIBS) $(OPT) $^ -o $@

%.o: %.cpp
	$(CPP) $(CFLAGS) $(OPT) -ferror-limit=1 -c $^ -o $@

run1: work1
	$(PROFILER)  ./$^ $(INPUTS)

run2: work2
	$(PROFILER)  ./$^ $(INPUTS)

run3: work3
	$(PROFILER)  ./$^ $(INPUTS)

run4: work4
	$(PROFILER)  ./$^ $(INPUTS)

clean:
	rm -f *.o perf.* $(PROGRAMS)

.PHONY: clean run
