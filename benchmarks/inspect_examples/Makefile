CC  = gcc
CXX  = g++

CCFLAGS = -O3 #-g

LIBDIR = ../lib
INCLUDE_DIR = ../src

LIB_FLAG = -L $(LIBDIR)
INCLUDE_FLAG = -I $(INCLUDE_DIR)

all:  sched_make  simp1   


lb :
	make -C ../src lb
	mv ../src/balancer .

sched_make :
	make -C ../src
	mv ../src/checker .


robot:  exec_robot.cc
	$(CXX) $(CCFLAGS) -o robot exec_robot.cc


simp1:  simple1.instr.o 
	$(CXX) -o test  simple1.instr.o  \
	   $(LIB_FLAG) -linspector -lpthread


simp2:  simple2.instr.o 
	$(CXX) -o test  simple2.instr.o  \
           $(LIB_FLAG) -linspector -lpthread


simp3:  simple3.instr.o 
	$(CXX) -o test3  simple3.instr.o  \
           $(LIB_FLAG) -linspector -lpthread


dpor1:  clean  dpor-example1.instr2.o
	$(CXX) -o dpor1  dpor-example1.instr2.o \
           $(LIB_FLAG) -linspector -lpthread


dpor2:  clean  dpor-example2.instr.o
	$(CXX) -o dpor2  dpor-example2.instr.o \
           $(LIB_FLAG) -linspector -lpthread

dpor3:  clean  dpor-example3.instr.o
	$(CXX) -o dpor3  dpor-example3.instr.o \
           $(LIB_FLAG) -linspector -lpthread


cond1:  clean  cond1.instr.o
	$(CXX) -o cond1  cond1.instr.o \
           $(LIB_FLAG) -linspector -lpthread


cond2:  clean  cond2.instr.o
	$(CXX) -o cond1  cond2.instr.o \
           $(LIB_FLAG) -linspector -lpthread


rwlock1: rwlock1.instr.c
	$(CXX) -D INSPECTOR_RWLOCK $(CCFLAGS) -c $^ $(INCLUDE_FLAG)\
               -o $@.instr.o
	$(CXX) -o $@  $@.instr.o  $(LIB_FLAG) -linspector -lpthread


bbuf:  clean boundedBuffer.instr.o
	$(CXX) -o bbuf  boundedBuffer.instr.o \
	   $(LIB_FLAG) -linspector -lpthread

phil:  clean philosopher.instr.o
	$(CXX) -o phil  philosopher.instr.o  \
	   $(LIB_FLAG) -linspector -lpthread


err1:  error1.instr.o 
	$(CXX) -o err1  error1.instr.o  \
	   $(LIB_FLAG) -linspector -lpthread

err2:  error2.instr.o 
	$(CXX) -o err2  error2.instr.o  \
	   $(LIB_FLAG) -linspector -lpthread


err3:  error3.instr.o 
	$(CXX) -o err3  error3.instr.o  \
	   $(LIB_FLAG) -linspector -lpthread


err4:  error4.instr.o 
	$(CXX) -o err4  error4.instr.o  \
	   $(LIB_FLAG) -linspector -lpthread


bug1:  bug1.instr.o
	$(CXX) -o bug1 bug1.instr.o  \
	  $(LIB_FLAG) -linspector -lpthread


bug2:  bug2.instr.o
	$(CXX) -o bug2  bug2.instr.o  \
	  $(LIB_FLAG) -linspector -lpthread

bug3:  bug3.instr.o
	$(CXX) -o bug3  bug3.instr.o  \
	  $(LIB_FLAG) -linspector -lpthread


carter01: carter01.instr.o
	$(CXX) -o $@  $^  $(LIB_FLAG) -linspector -lpthread


myex: myex.instr.o
	$(CXX) -o $@  $^  $(LIB_FLAG) -linspector -lpthread


s5: simple5.instr.o
	$(CXX) -o $@  $^  $(LIB_FLAG) -linspector -lpthread



bench:  bench.instr.o
	$(CXX) -o $@  $^  $(LIB_FLAG) -linspector -lpthread

bench2:  bench2.instr.o
	$(CXX) -o $@  $^  $(LIB_FLAG) -linspector -lpthread

ob:  bench.o
	$(CXX) -o $@  $^  $(LIB_FLAG) -lpthread

ob2:  bench2.o
	$(CXX) -o $@  $^  $(LIB_FLAG) -linspector -lpthread


aget:  aget-0.4.comb.instr.o
	$(CXX) -o $@  $^  $(LIB_FLAG) -linspector -lpthread


libclean:
	make -C ../src/  clean

clean:  
	rm -f *.o 


%.o : %.cc
	$(CXX) $(CCFLAGS) -c $^ $(INCLUDE_FLAG) -o $@

%.o : %.c
	$(CXX) $(CCFLAGS) -c $^ $(INCLUDE_FLAG) -o $@


