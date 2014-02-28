////////////////////////////////////////////////////////////////////////////////
// Timer Interface
// Author: Christopher Celio
// Date  : 2011 May 13

// Abstract out the timer code from each micro-benchmark.

#include "cctimer.h"
              
// ignore input "clk_freq", as we do not need it to 
// return the time in seconds
cctime_t inline cc_get_seconds(double clk_freq)
{
   struct timeval {
      long tv_sec;
      long tv_usec;
   };
 
   struct timezone {
      int tz_minuteswest;
      int tz_dsttime;
   };
 
   struct timeval tp;
   struct timezone tzp;
 
   gettimeofday(&tp,&tzp);
   return ( (double) tp.tv_sec + (double) tp.tv_usec * 1.e-6 );
}
              
cccycles_t inline cc_get_cycles(double clk_freq)
{
   struct timeval {
      long tv_sec;
      long tv_usec;
   };
 
   struct timezone {
      int tz_minuteswest;
      int tz_dsttime;
   };
 
   struct timeval tp;
   struct timezone tzp;
 
   gettimeofday(&tp,&tzp);
   double seconds = ( (double) tp.tv_sec + (double) tp.tv_usec * 1.e-6 );
   return seconds * clk_freq;
}
        
