TARGET=caches
SOURCES:=caches.c 

# <Num Cores. <Num Elements Per Core> <Num Iterations>
RUN_TYPE = 0
ARRAY_SIZE = 16
ARRAY_SIZE = 128
NUM_ITERATIONS = 10
APP_FLAGS ?=$(ARRAY_SIZE) $(NUM_ITERATIONS) $(RUN_TYPE)

include ../common/Makefile.tests
