##########################################
# Architecture-specific Makefile fragment
##########################################

CC=gcc

# for now, compile common/barrier.* using our own barrier implementations.
# if your compiler cant handle march, you're using an older version of gcc
CFLAGS=-std=c99 -mtune=native -march=native -mssse3 -O3 -funroll-loops -I../common -DWITH_BARRIER -Wall
	
# OSX users with the older gcc4.2, doesn't support march flag
ifeq ($(shell gcc -v 2>&1 | tail -n1 | awk '{print $$3}'),4.2.1)
	CFLAGS=-std=c99 -mtune=native -O3 -funroll-loops -I../common -DWITH_BARRIER -Wall
endif

LD_FLAGS=
LD_LIBS=-lpthread -lm
 
