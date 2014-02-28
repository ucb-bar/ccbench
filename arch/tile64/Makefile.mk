##########################################
# Architecture-specific Makefile fragment
##########################################

# Tilera flags
BIN = $(TILERA_ROOT)/bin/

CC=$(BIN)tile-cc
CFLAGS=-std=c99 -O3 -OPT:unroll_analysis=on -CG:unroll_multi_bb=on -OPT:unroll_times_max=4 -I../common -DWITH_BARRIER -DTILERA_ARCH

LD_FLAGS=
LD_LIBS=-lpthread -lilib -lm
 

TILE_MONITOR = $(BIN)tile-monitor

EXECUTABLE = $(TARGET)

MONITOR_COMMON_ARGS = \
  --batch-mode \
  --mkdir /opt/test \
  --cd /opt/test \
  --upload $(EXECUTABLE) $(EXECUTABLE) \
  -- ./$(EXECUTABLE) $(APP_FLAGS)
#  -- mcstat -c ./$(EXECUTABLE) $(APP_FLAGS)

# For faster execution, also specify the "--functional" option to run
# the simulator in functional mode, as opposed to the default
# timing-accurate mode.
SIMULATOR_ARGS = \
  --image tile64 \
  --tile 4x4
#  --image 4x4 \
  --tile 4x2                 

#tile option just specifies affinity
PCI_ARGS = \
  --pci-resume \
  --tile 8x7 \
  --hvc ../common/striped-vmlinux-pci.hvc
#  --tile 4x2


#tile option just specifies affinity
PCI_BOOT_ARGS = \
  --pci \
  --tile 8x7 \
  --hvc ../common/striped-vmlinux-pci.hvc
 

runtilesim: $(EXECUTABLE)
	$(TILE_MONITOR) $(SIMULATOR_ARGS) $(MONITOR_COMMON_ARGS)

runtile: $(EXECUTABLE)
	$(TILE_MONITOR) $(PCI_ARGS) $(MONITOR_COMMON_ARGS)
 
boottile: $(EXECUTABLE)
	$(TILE_MONITOR) $(PCI_BOOT_ARGS) $(MONITOR_COMMON_ARGS)
   
 
 
