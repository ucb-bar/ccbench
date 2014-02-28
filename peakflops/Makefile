TARGET=peakflops
SOURCES:=peakflops.c 

#APP_FLAGS ?=
PROCESSORNAME?="merom"

include ../common/Makefile.tests

run-benchmark: $(TARGET)
	./$(TARGET) $(PROCESSORNAME) >> results.txt
	cat results.txt
