////////////////////////////////////////////////////////////////////////////////
// Timer Interface
// Author: Christopher Celio
// Date  : 2011 May 13

// Abstract out the timer code from each micro-benchmark.

#include <stdint.h>
#include <time.h>

// unit is "seconds"
typedef double cctime_t;

// Input clk_freq is ignored, unless the underlying timer
// outputs in cycles.  Then the conversion to seconds is 
// required.
cctime_t cc_get_seconds(double clk_freq);

// unit is "cycles"
typedef uint32_t cccycles_t;

// converts from seconds to cycles, given the frequency
// if a cycle-accurate timer is available, then the clk_freq
// input is ignored.
cccycles_t cc_get_cycles(double clk_freq);
