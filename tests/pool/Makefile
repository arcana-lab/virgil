CPP=clang++
CFLAGS=-std=c++14 -g -I../../include
LIBS=-pthreads
PROGRAMS=test1 test2 test3 test4 test5 test6 testHELIX baseline test_extendible stresstest1
PROGRAM=baseline
PROFILER=/usr/bin/time taskset -c 1
PROFILER_PERF=perf record --call-graph fp -e cpu-clock taskset -c 1
PROFILER_SHOW=perf report --stdio
OPT=-O3 -fno-inline -fno-omit-frame-pointer
INPUTS=100000000 0 14

all: $(PROGRAMS)

test1: test1.o
	$(CPP) $(LIBS) $(OPT) $^ -o $@

test2: test2.o
	$(CPP) $(LIBS) $(OPT) $^ -o $@

test3: test3.o
	$(CPP) $(LIBS) $(OPT) $^ -o $@

test4: test4.o
	$(CPP) $(LIBS) $(OPT) $^ -o $@

test5: test5.o
	$(CPP) $(LIBS) $(OPT) $^ -o $@

test6: test6.o
	$(CPP) $(LIBS) $(OPT) $^ -o $@

testHELIX: testHELIX.o
	$(CPP) $(LIBS) $(OPT) $^ -o $@

test_extendible: test_extendible.o
	$(CPP) $(LIBS) $(OPT) $^ -o $@

baseline: baseline.o
	$(CPP) $(LIBS) $(OPT) $^ -o $@

stresstest1: stresstest1.o 
	$(CPP) $(LIBS) $(OPT) $^ -o $@

%.o: %.cpp
	$(CPP) $(CFLAGS) $(OPT) -c $^ -o $@

performance: stresstest1
	perf stat ./$< 8 300000 8

run: $(PROGRAM)
	$(PROFILER)  ./$(PROGRAM) $(INPUTS)

clean:
	rm -f *.o $(PROGRAMS) perf.* *.txt

.PHONY: clean run performance
