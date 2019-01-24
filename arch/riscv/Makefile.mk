##########################################
# Architecture-specific Makefile fragment
##########################################

CC=riscv64-unknown-linux-gnu-gcc

# for now, compile common/barrier.* using our own barrier implementations.
# if your compiler cant handle march, you're using an older version of gcc
#CFLAGS=-Wa,-march=RVIMAFDXhwacha -std=gnu99 -O2 -ffast-math -funroll-loops -I../common 
CFLAGS=-std=gnu99 -O2 -ffast-math -funroll-loops -I../common 

#CFLAGS=-std=c99 -mtune=native -march=native -mssse3 -O3 -funroll-loops -I../common -DWITH_BARRIER -Wall
	
LD_FLAGS= -ffast-math
LD_LIBS=-lm -lc

