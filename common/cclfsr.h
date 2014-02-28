////////////////////////////////////////////////////////////////////////////////
// Linear Feedback Shift Register (LFSR) Interface
// Author: Christopher Celio
// Date  : 2011 Jun 24

// Used for generating a sequence of random numbers for use in randomizing
// arrays of pointers very, very quickly. 
//
// Current Drawbacks:
//    only provides one set of taps, as opposed to a choice of different possible sequences
//    requires the user to handle randomizing for non-powers of twos

#include <stdint.h>
#include <stdio.h>

#define LFSR_MAX_TAPS (10)

typedef struct
{
   uint32_t value;
   uint32_t width;
   uint32_t mask;
   uint32_t taps[LFSR_MAX_TAPS]; //allocate worst case space for the taps array
}
cc_lfsr_t;


// initializes lfsr->width, lfsr->mask, lfsr->taps, and lfsr->value
uint32_t cc_lfsr_init(cc_lfsr_t* lfsr, uint32_t init_val, uint32_t width);

// returns next value to set lfsr.value to
// no side effects to lfsr are performed
uint32_t cc_lfsr_next(cc_lfsr_t* lfsr);


