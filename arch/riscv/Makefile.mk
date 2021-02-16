##########################################
# Architecture-specific Makefile fragment
##########################################

CC=riscv64-unknown-elf-gcc

# for now, compile common/barrier.* using our own barrier implementations.
# if your compiler cant handle march, you're using an older version of gcc
CFLAGS= -std=gnu99 -O2 -ffast-math -funroll-loops -I../common -mcmodel=medany -g
#CFLAGS=-std=c99 -mtune=native -march=native -mssse3 -O3 -funroll-loops -I../common -DWITH_BARRIER -Wall
	
LD_FLAGS= -ffast-math -static
LD_LIBS=-lm -lc

