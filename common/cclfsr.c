////////////////////////////////////////////////////////////////////////////////
// Linear Feedback Shift Register (LFSR) Interface
// Author: Christopher Celio
// Date  : 2011 Jun 24

// taps for m-sequences taken from the following links:
// http://cfn.upenn.edu/aguirre/code/matlablib/mseq/mseq.m
// http://www.xilinx.com/support/documentation/application_notes/xapp052.pdf
//
#include "cclfsr.h"

uint32_t get_bit(uint32_t lfsr, uint32_t pos)
{
   return (lfsr >> pos) & 0x1;
}

//TODO rewrite for arbitrary masks, since we don't care about speed anymore
//(only run this once at init)
uint32_t get_mask(uint32_t width)
{
   uint32_t mask = 0;

   switch(width) 
   {
      case 16: mask = 0xffff; break;
      case 15: mask = 0x7fff; break;
      case 14: mask = 0x3fff; break;
      case 13: mask = 0x1fff; break;
      case 12: mask = 0x0fff; break;
      case 11: mask = 0x07ff; break;
      case 10: mask = 0x03ff; break;
      case  9: mask = 0x01ff; break;
      case  8: mask = 0x00ff; break;
      case  7: mask = 0x007f; break;
      case  6: mask = 0x003f; break;
      case  5: mask = 0x001f; break;
      case  4: mask = 0x000f; break;
      case  3: mask = 0x0007; break;
      case  2: mask = 0x0003; break;
      case  1: mask = 0x0001; break;
      default: mask = 0x0;
         printf("ERROR: cclfsr.c - invalid width for mask generation\n"); 
      break;
   }

   return mask;
}



// TODO add more taps...
// end the array with a -1 as our 'null' terminator
static uint32_t taps12[5] = {11,10, 7, 5,  -1};
static uint32_t taps11[3] = {10, 8,  -1};
static uint32_t taps10[5] = { 9, 4, 2, 1,  -1};
static uint32_t taps9[5]  = { 8, 6, 1, 0,  -1};
static uint32_t taps8[5]  = { 7, 6, 1, 0,  -1};
static uint32_t taps7[5]  = { 6, 4, 1, 0,  -1};

static uint32_t taps6[5]  = { 5, 4, 2, 1,  -1};
//static uint32_t taps6[5]  = { 5, 4, 3, 0,  -1};
//static uint32_t taps6[5]  = { 5, 3, 2, 0,  -1};
//static uint32_t taps6[3]  = { 5, 4,  -1};

static uint32_t taps5[3]  = { 4, 1,  -1};
static uint32_t taps4[3]  = { 3, 0,  -1};
static uint32_t taps3[3]  = { 2, 0,  -1};
static uint32_t taps2[3]  = { 1, 0,  -1};
static uint32_t tapsdefault[2]  = {0,  -1};


uint32_t* get_taps(uint32_t width)
{
   switch(width) 
   {
      case 12: return taps12;
      case 11: return taps11;
      case 10: return taps10;
      case  9: return taps9;
      case  8: return taps8;
      case  7: return taps7;
      case  6: return taps6;
      case  5: return taps5;
      case  4: return taps4;
      case  3: return taps3;
      case  2: return taps2;
      default:
         printf("ERROR: cclfsr.c - invalid width for taps generation, input width=%d\n",
            width); 
         return tapsdefault;
   }
}


uint32_t cc_lfsr_init(cc_lfsr_t* lfsr, uint32_t init_val, uint32_t width)
{
   lfsr->value = init_val;
   lfsr->width = width;
   lfsr->mask  = get_mask(width);
   uint32_t* taps = get_taps(width);
   
   uint32_t idx = 0;

   while(taps[idx] != -1 && idx < LFSR_MAX_TAPS)
   {
      lfsr->taps[idx] = taps[idx];
      idx++;
   }

   if (idx >= LFSR_MAX_TAPS)
      printf("ERROR: cclfsr.c - missing null terminator in taps specification?\n"); 
   
   // add the last "-1" to the array too
   lfsr->taps[idx] = taps[idx];

   return 0;
}


uint32_t cc_lfsr_next(cc_lfsr_t* lfsr)
{
   uint32_t  current = lfsr->value;
   uint32_t feedback = 0;


   uint32_t idx = 0;
   while(lfsr->taps[idx] != -1)
   {
      feedback ^= get_bit(current, lfsr->taps[idx]);

      idx++;
   }
   
   
   uint32_t next = ((current << 1) & lfsr->mask) | (feedback & 0x1);

   return next;
}


