##########################################
# Architecture-specific Makefile fragment
##########################################

CC=gcc

# for now, compile common/barrier.* using our own barrier implementations.
# if your compiler cant handle march, you're using an older version of gcc
CFLAGS=-std=c99 -mtune=native -O3 -funroll-loops -I../common -DWITH_BARRIER -Wall
	
LD_FLAGS=
LD_LIBS=-lpthread -lm
 
