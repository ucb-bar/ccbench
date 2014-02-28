TARGET=incluexclu
SOURCES:=incluexclu.c 

# <Num Threads> <Num Elements Per Core> <Num Iterations>
NUM_THREADS = 1
ARRAY_SIZE = 8192
NUM_ITERATIONS = 100
APP_FLAGS ?=$(NUM_THREADS) $(ARRAY_SIZE) $(NUM_ITERATIONS) 

include ../common/Makefile.tests
