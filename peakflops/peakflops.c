////////////////////////////////////////////////////////////////////////////////
// PeakFlops (compute empirically the maximum flop performance of the machine)
// Author: Christopher Celio
// Date  : 2011 Nov 21
//

//#define DEBUG 
#define PRINT_SCRIPT_FRIENDLY
 

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <cctimer.h>

#include <pmmintrin.h> //sse3

// Global Variables
uint32_t  g_num_iterations = 100000000;
uint32_t  g_performed_iterations;
char*     proc_string;


#define NUM_TESTS (5)

uint64_t g_num_flop[NUM_TESTS];
double volatile run_time_s[NUM_TESTS];

#define ADDMUL_IDX (0)
#define ADD_IDX (1)
#define SIMDADDMUL_IDX (2)
#define SIMDADD_IDX (3)
#define VMAG_IDX (4)


/***********************************/
/*          Test Functions         */
/***********************************/
 
// return run_time_s, done in such a manner as to save ourselves xmm registers
// ( using globals took up xmm registers :( )
// placebo is just a way to keep the timed computation from getting removed
// by the compiler
double TestAddMul_1t1(double* placebo)
{
   // clk_freq irrelevant 
   uint32_t const clk_freq = 0;
   g_performed_iterations = g_num_iterations;

   //only using 15 registers, since xmm0 is getting used for ... something
   //don't want to spill to the stack!
   double register c0;
   double register c1;
   double register c2;
   double register c3;
   double register c4;
   double register c5;
   double register c6;
   double register c7;
   double register c8;
   double register c9;
   double register c10;
   double register c11;
   double register c12;
   double register c13;
   double register c14;
   
   // 15 registers, two flop per register
   const uint64_t num_flop_per_loop = 15*2;

   srand(time(NULL));

   c0 = drand48( );
   c1 = c0 + 1.0;
   c2 = c1 + 1.0;
   c3 = c2 + 1.0;
   c4 = c3 + 1.0;
   c5 = c4 + 1.0;
   c6 = c5 + 1.0;
   c7 = c6 + 1.0;
   c8 = c7 + 1.0;
   c9 = c8 + 1.0;
   c10 = c9 + 1.0;
   c11 = c10 + 1.0;
   c12 = c11 + 1.0;
   c13 = c12 + 1.0;
   c14 = c13 + 1.0;

   /** CRITICAL SECTION **/

   // run for g_num_iterations...
   cctime_t volatile start_time = cc_get_seconds(clk_freq);

   for (uint32_t k = 0; k < g_num_iterations; k++)
   { //this gets ~2*clk_rate GF/s  ( 4.6 GF/s on my 2.3 Ghz machine :D )
       c0 = c0 + c0;       
       c0 = c0 * c0;
       c1 = c1 + c1;
       c1 = c1 * c1;
       c2 = c2 + c2;
       c2 = c2 * c2;
       c3 = c3 + c3;
       c3 = c3 * c3;
       c4 = c4 + c4;
       c4 = c4 * c4;
       c5 = c5 + c5;
       c5 = c5 * c5;
       c6 = c6 + c6;
       c6 = c6 * c6;
       c7 = c7 + c7;
       c7 = c7 * c7;
       c8 = c8 + c8;
       c8 = c8 * c8;
       c9 = c9 + c9;
       c9 = c9 * c9;
       c10 = c10 + c10;
       c10 = c10 * c10;
       c11 = c11 + c11;
       c11 = c11 * c11;
       c12 = c12 + c12;
       c12 = c12 * c12;
       c13 = c13 + c13;
       c13 = c13 * c13;
       c14 = c14 + c14;
       c14 = c14 * c14;
   }

   cctime_t volatile stop_time = cc_get_seconds(clk_freq);
   double run_time_s = ((double) (stop_time - start_time)); 

   g_num_flop[ADDMUL_IDX] = num_flop_per_loop * g_num_iterations;



   // prevent compiler from removing computation...
   // although the receiver must put placebo into a volatile variable as well!
   
   *placebo = (double) c0 
         + (double) c1 
         + (double) c2 
         + (double) c3 
         + (double) c4 
         + (double) c5 
         + (double) c6 
         + (double) c7
         + (double) c8
         + (double) c9
         + (double) c10
         + (double) c11
         + (double) c12
         + (double) c13
         + (double) c14
         ;
   
   return run_time_s;
}

double TestAdd(double* placebo)
{
   // clk_freq irrelevant 
   uint32_t const clk_freq = 0;
   g_performed_iterations = g_num_iterations;

   //only using 15 registers, since xmm0 is getting used for ... something
   //don't want to spill to the stack!
   double register c0;
   double register c1;
   double register c2;
   double register c3;
   double register c4;
   double register c5;
   double register c6;
   double register c7;
   double register c8;
   double register c9;
   double register c10;
   double register c11;
   double register c12;
   double register c13;
   double register c14;
   
   // 15 registers, two flop per register
   const uint64_t num_flop_per_loop = 15;

   srand(time(NULL));

   c0 = drand48( );
   c1 = c0 + 1.0;
   c2 = c1 + 1.0;
   c3 = c2 + 1.0;
   c4 = c3 + 1.0;
   c5 = c4 + 1.0;
   c6 = c5 + 1.0;
   c7 = c6 + 1.0;
   c8 = c7 + 1.0;
   c9 = c8 + 1.0;
   c10 = c9 + 1.0;
   c11 = c10 + 1.0;
   c12 = c11 + 1.0;
   c13 = c12 + 1.0;
   c14 = c13 + 1.0;
    

   /** CRITICAL SECTION **/

   // run for g_num_iterations...
   cctime_t volatile start_time = cc_get_seconds(clk_freq);

   for (uint32_t k = 0; k < g_num_iterations; k++)
   { 
       c0 = c0 + c0;       
       c1 = c1 + c1;
       c2 = c2 + c2;
       c3 = c3 + c3;
       c4 = c4 + c4;
       c5 = c5 + c5;
       c6 = c6 + c6;
       c7 = c7 + c7;
       c8 = c8 + c8;
       c9 = c9 + c9;
       c10 = c10 + c10;
       c11 = c11 + c11;
       c12 = c12 + c12;
       c13 = c13 + c13;
       c14 = c14 + c14;
   }

   cctime_t volatile stop_time = cc_get_seconds(clk_freq);
   double run_time_s = ((double) (stop_time - start_time)); 

   g_num_flop[ADD_IDX] = num_flop_per_loop * g_num_iterations;



   // prevent compiler from removing computation...
   // although the receiver must put placebo into a volatile variable as well!
   *placebo= (double) c0 
         + (double) c1 
         + (double) c2 
         + (double) c3 
         + (double) c4 
         + (double) c5 
         + (double) c6 
         + (double) c7
         + (double) c8
         + (double) c9
         + (double) c10
         + (double) c11
         + (double) c12
         + (double) c13
         + (double) c14
         ;
   
   return run_time_s;
}
 
// return run_time_s, done in such a manner as to save ourselves xmm registers
// ( using globals took up xmm registers :( )
// placebo is just a way to keep the timed computation from getting removed
// by the compiler
double TestSimdAddMul_1t1(double* placebo)
{
   // clk_freq irrelevant 
   uint32_t const clk_freq = 0;
   g_performed_iterations = g_num_iterations;

   __m128d v_c0  = {1.0, 0.0}; 
   __m128d v_c1  = {2.0, 1.0}; 
   __m128d v_c2  = {3.0, 2.0}; 
   __m128d v_c3  = {4.0, 3.0}; 
   __m128d v_c4  = {5.0, 4.0}; 
   __m128d v_c5  = {6.0, 5.0}; 
   __m128d v_c6  = {7.0, 6.0}; 
   __m128d v_c7  = {8.0, 7.0}; 
   __m128d v_c8  = {9.0, 8.0}; 
   __m128d v_c9  = {10.0, 9.0}; 
   __m128d v_c10  = {11.0, 10.0}; 
   __m128d v_c11  = {12.0, 11.0}; 
   __m128d v_c12  = {13.0, 12.0}; 
   __m128d v_c13  = {14.0, 13.0}; 
   __m128d v_c14  = {15.0, 14.0}; 
//   __m128d v_c15  = {16.0, 15.0}; 
    

   // xxx registers, 2 doubles per register, 2 flop per register
   const uint64_t num_flop_per_loop = 15*2*2;

   /** CRITICAL SECTION **/

   // run for g_num_iterations...
   cctime_t volatile start_time = cc_get_seconds(clk_freq);

   for (uint32_t k = 0; k < g_num_iterations; k++)
   { //this gets ~2*clk_rate GF/s  ( 4.6 GF/s on my 2.3 Ghz machine :D )
       v_c0  = _mm_add_pd(v_c0, v_c0);
       v_c0  = _mm_mul_pd(v_c0, v_c0);
       v_c1  = _mm_add_pd(v_c1, v_c1);
       v_c1  = _mm_mul_pd(v_c1, v_c1);
       v_c2  = _mm_add_pd(v_c2, v_c2);
       v_c2  = _mm_mul_pd(v_c2, v_c2);
       v_c3  = _mm_add_pd(v_c3, v_c3);
       v_c3  = _mm_mul_pd(v_c3, v_c3);
       v_c4  = _mm_add_pd(v_c4, v_c4);
       v_c4  = _mm_mul_pd(v_c4, v_c4);
       v_c5  = _mm_add_pd(v_c5, v_c5);
       v_c5  = _mm_mul_pd(v_c5, v_c5);
       v_c6  = _mm_add_pd(v_c6, v_c6);
       v_c6  = _mm_mul_pd(v_c6, v_c6);
       v_c7  = _mm_add_pd(v_c7, v_c7);
       v_c7  = _mm_mul_pd(v_c7, v_c7);
       v_c8  = _mm_add_pd(v_c8, v_c8);
       v_c8  = _mm_mul_pd(v_c8, v_c8);
       v_c9  = _mm_add_pd(v_c9, v_c9);
       v_c9  = _mm_mul_pd(v_c9, v_c9);
       v_c10 = _mm_add_pd(v_c10, v_c10);
       v_c10 = _mm_mul_pd(v_c10, v_c10);
       v_c11 = _mm_add_pd(v_c11, v_c11);
       v_c11 = _mm_mul_pd(v_c11, v_c11);
       v_c12 = _mm_add_pd(v_c12, v_c12);
       v_c12 = _mm_mul_pd(v_c12, v_c12);
       v_c13 = _mm_add_pd(v_c13, v_c13);
       v_c13 = _mm_mul_pd(v_c13, v_c13);
       v_c14 = _mm_add_pd(v_c14, v_c14);
       v_c14 = _mm_mul_pd(v_c14, v_c14);
//       v_c15 = _mm_add_pd(v_c15, v_c15);
//       v_c15 = _mm_mul_pd(v_c15, v_c15);
   }

   cctime_t volatile stop_time = cc_get_seconds(clk_freq);
   double run_time_s = ((double) (stop_time - start_time)); 

   g_num_flop[SIMDADDMUL_IDX] = num_flop_per_loop * g_num_iterations;



   // prevent compiler from removing computation...
   // although the receiver must put placebo into a volatile variable as well!
   
   double ret0_0, ret0_1;
   double ret1_0, ret1_1;
   double ret2_0, ret2_1;
   double ret3_0, ret3_1;
   double ret4_0, ret4_1;
   double ret5_0, ret5_1;
   double ret6_0, ret6_1;
   double ret7_0, ret7_1;
   double ret8_0, ret8_1;
   double ret9_0, ret9_1;
   double ret10_0, ret10_1;
   double ret11_0, ret11_1;
   double ret12_0, ret12_1;
   double ret13_0, ret13_1;
   double ret14_0, ret14_1;
//   double ret15_0, ret15_1;
   
   _mm_store_sd (&ret0_0, v_c0);
   _mm_storeh_pd(&ret0_1, v_c0);
   _mm_store_sd (&ret1_0, v_c1);
   _mm_storeh_pd(&ret1_1, v_c1);
   _mm_store_sd (&ret2_0, v_c2);
   _mm_storeh_pd(&ret2_1, v_c2);
   _mm_store_sd (&ret3_0, v_c3);
   _mm_storeh_pd(&ret3_1, v_c3);
   _mm_store_sd (&ret4_0, v_c4);
   _mm_storeh_pd(&ret4_1, v_c4);
   _mm_store_sd (&ret5_0, v_c5);
   _mm_storeh_pd(&ret5_1, v_c5);
   _mm_store_sd (&ret6_0, v_c6);
   _mm_storeh_pd(&ret6_1, v_c6);
   _mm_store_sd (&ret7_0, v_c7);
   _mm_storeh_pd(&ret7_1, v_c7);
   _mm_store_sd (&ret8_0, v_c8);
   _mm_storeh_pd(&ret8_1, v_c8);
   _mm_store_sd (&ret9_0, v_c9);
   _mm_storeh_pd(&ret9_1, v_c9);
   _mm_store_sd (&ret10_0, v_c10);
   _mm_storeh_pd(&ret10_1, v_c10);
   _mm_store_sd (&ret11_0, v_c11);
   _mm_storeh_pd(&ret11_1, v_c11);
   _mm_store_sd (&ret12_0, v_c12);
   _mm_storeh_pd(&ret12_1, v_c12);
   _mm_store_sd (&ret13_0, v_c13);
   _mm_storeh_pd(&ret13_1, v_c13);
   _mm_store_sd (&ret14_0, v_c14);
   _mm_storeh_pd(&ret14_1, v_c14);
//   _mm_store_sd (&ret15_0, v_c15);
//   _mm_storeh_pd(&ret15_1, v_c15);
   
   *placebo = 
           ret0_0 + ret0_1
         + ret1_0 + ret1_1
         + ret2_0 + ret2_1
         + ret3_0 + ret3_1
         + ret4_0 + ret4_1
         + ret5_0 + ret5_1
         + ret6_0 + ret6_1
         + ret7_0 + ret7_1
         + ret8_0 + ret8_1
         + ret9_0 + ret9_1
         + ret10_0 + ret10_1
         + ret11_0 + ret11_1
         + ret12_0 + ret12_1
         + ret13_0 + ret13_1
         + ret14_0 + ret14_1
//         + ret15_0 + ret15_1
         ;
   
   
   return run_time_s;
}
 
// return run_time_s, done in such a manner as to save ourselves xmm registers
// ( using globals took up xmm registers :( )
// placebo is just a way to keep the timed computation from getting removed
// by the compiler
double TestSimdAdd(double* placebo)
{
   // clk_freq irrelevant 
   uint32_t const clk_freq = 0;
   g_performed_iterations = g_num_iterations;

// _mm_set_ps1  set value to other positions?
   __m128d v_c0 = {1.0, 0.0};
   __m128d v_c1 = {2.0, 1.0};
   __m128d v_c2 = {3.0, 2.0};
   __m128d v_c3 = {4.0, 3.0};
   __m128d v_c4 = {5.0, 4.0};
   __m128d v_c5 = {6.0, 5.0};
   __m128d v_c6 = {7.0, 6.0};
   __m128d v_c7 = {8.0, 7.0};
   __m128d v_c8 = {9.0, 8.0};
//   __m128d v_c9 = {10.0, 9.0};
//   __m128d v_c10 = {11.0, 10.0};
//   __m128d v_c11 = {12.0, 11.0};
//   __m128d v_c12 = {13.0, 12.0};
//   __m128d v_c13 = {14.0, 13.0};
//   __m128d v_c14 = {15.0, 14.0};
//   __m128d v_c15 = {16.0, 15.0};


   // xxx registers, 2 doubles per register, 1 flop per register
   const uint64_t num_flop_per_loop = 9*2*1;

//   srand(time(NULL));
//   double volatile input = 1.0;

   /** CRITICAL SECTION **/

   // run for g_num_iterations...
   cctime_t volatile start_time = cc_get_seconds(clk_freq);

   for (uint32_t k = 0; k < g_num_iterations; k++)
   { 
       v_c0  = _mm_add_pd(v_c0, v_c0);
       v_c1  = _mm_add_pd(v_c1, v_c1);
       v_c2  = _mm_add_pd(v_c2, v_c2);
       v_c3  = _mm_add_pd(v_c3, v_c3);
       v_c4  = _mm_add_pd(v_c4, v_c4);
       v_c5  = _mm_add_pd(v_c5, v_c5);
       v_c6  = _mm_add_pd(v_c6, v_c6);
       v_c7  = _mm_add_pd(v_c7, v_c7);
       v_c8  = _mm_add_pd(v_c8, v_c8);
//       v_c9  = _mm_add_pd(v_c9, v_c9);
//       v_c10 = _mm_add_pd(v_c10, v_c10);
//       v_c11 = _mm_add_pd(v_c11, v_c11);
//       v_c12 = _mm_add_pd(v_c12, v_c12);
//       v_c13 = _mm_add_pd(v_c13, v_c13);
//       v_c14 = _mm_add_pd(v_c14, v_c14);
//       v_c15 = _mm_add_pd(v_c15, v_c15);
   }

   cctime_t volatile stop_time = cc_get_seconds(clk_freq);
   double run_time_s = ((double) (stop_time - start_time)); 

   g_num_flop[SIMDADD_IDX] = num_flop_per_loop * g_num_iterations;


   // prevent compiler from removing computation...
   // although the receiver must put placebo into a volatile variable as well!
   
   double ret0_0, ret0_1;
   double ret1_0, ret1_1;
   double ret2_0, ret2_1;
   double ret3_0, ret3_1;
   double ret4_0, ret4_1;
   double ret5_0, ret5_1;
   double ret6_0, ret6_1;
   double ret7_0, ret7_1;
   double ret8_0, ret8_1;
   double ret9_0, ret9_1;
   double ret10_0, ret10_1;
   double ret11_0, ret11_1;
   double ret12_0, ret12_1;
   double ret13_0, ret13_1;
   double ret14_0, ret14_1;
//   double ret15_0, ret15_1;
   
   _mm_store_sd (&ret0_0, v_c0);
   _mm_storeh_pd(&ret0_1, v_c0);
   _mm_store_sd (&ret1_0, v_c1);
   _mm_storeh_pd(&ret1_1, v_c1);
   _mm_store_sd (&ret2_0, v_c2);
   _mm_storeh_pd(&ret2_1, v_c2);
   _mm_store_sd (&ret3_0, v_c3);
   _mm_storeh_pd(&ret3_1, v_c3);
   _mm_store_sd (&ret4_0, v_c4);
   _mm_storeh_pd(&ret4_1, v_c4);
   _mm_store_sd (&ret5_0, v_c5);
   _mm_storeh_pd(&ret5_1, v_c5);
   _mm_store_sd (&ret6_0, v_c6);
   _mm_storeh_pd(&ret6_1, v_c6);
   _mm_store_sd (&ret7_0, v_c7);
   _mm_storeh_pd(&ret7_1, v_c7);
   _mm_store_sd (&ret8_0, v_c8);
   _mm_storeh_pd(&ret8_1, v_c8);
//   _mm_store_sd (&ret9_0, v_c9);
//   _mm_storeh_pd(&ret9_1, v_c9);
//   _mm_store_sd (&ret10_0, v_c10);
//   _mm_storeh_pd(&ret10_1, v_c10);
//   _mm_store_sd (&ret11_0, v_c11);
//   _mm_storeh_pd(&ret11_1, v_c11);
//   _mm_store_sd (&ret12_0, v_c12);
//   _mm_storeh_pd(&ret12_1, v_c12);
//   _mm_store_sd (&ret13_0, v_c13);
//   _mm_storeh_pd(&ret13_1, v_c13);
//   _mm_store_sd (&ret14_0, v_c14);
//   _mm_storeh_pd(&ret14_1, v_c14);
//   _mm_store_sd (&ret15_0, v_c15);
//   _mm_storeh_pd(&ret15_1, v_c15);
   
   *placebo = 
           ret0_0 + ret0_1
         + ret1_0 + ret1_1
         + ret2_0 + ret2_1
         + ret3_0 + ret3_1
         + ret4_0 + ret4_1
         + ret5_0 + ret5_1
         + ret6_0 + ret6_1
         + ret7_0 + ret7_1
         + ret8_0 + ret8_1
//         + ret9_0 + ret9_1
//         + ret10_0 + ret10_1
//         + ret11_0 + ret11_1
//         + ret12_0 + ret12_1
//         + ret13_0 + ret13_1
//         + ret14_0 + ret14_1
//         + ret15_0 + ret15_1
         ;
   
   return run_time_s;
}
 

// test out substainable flop performance on an array that fits in X level of the cache hierarchy
// do a sum/mul reduction, with one load, no store on the array for max performance
// magnittude of vector (adds and multiplies)
// array = a0*a0 + a1*a1 + ..... an*an;
 double TestVMagnitude(uint32_t array_sz_el, double* placebo)
 {
   // clk_freq irrelevant 
   uint32_t const clk_freq = 0;
   g_performed_iterations = g_num_iterations;

   //only using 15 registers, since xmm0 is getting used for ... something
   //don't want to spill to the stack!
   double register c0  = 0.0;
   double register c1  = 0.0;
   double register c2  = 0.0;
   double register c3  = 0.0;
   double register c4  = 0.0;
   double register c5  = 0.0;
   double register c6  = 0.0;
   double register c7  = 0.0;
   double register c8  = 0.0;
   double register c9  = 0.0;
   double register c10 = 0.0;
   double register c11 = 0.0;
   double register c12 = 0.0;
   double register c13 = 0.0;
   double register c14 = 0.0;
//   double register c15 = 0.0;
   
   double register t0;
   double register t1;
   double register t2;
   double register t3;
   double register t4;
   double register t5;
   double register t6;
   double register t7;
   
   // 15 registers, two flop per register
   int num_registers = 15;
   const uint64_t num_flop_per_loop = num_registers*2;

   srand(time(NULL));

   double *array = (double*) malloc(array_sz_el*sizeof(double));

   for (int i=0; i < array_sz_el; i++)
      array[i] = drand48();


   /** CRITICAL SECTION **/

   // run for g_num_iterations...
   cctime_t volatile start_time = cc_get_seconds(clk_freq);

   // TODO run this for a longer period of time?
      for (uint32_t i = 0; i < array_sz_el; i+=(num_registers))
      { 
#if 0
          t0 = array[i];
          t1 = array[i+1]; 
          t2 = array[i+2]; 
          t3 = array[i+3]; 
          t4 = array[i+4]; 
          t5 = array[i+5]; 
          t6 = array[i+6]; 
          t7 = array[i+7]; 
          
          c0 += t0 * t0; 
          c1 += t1 * t1; 
          c2 += t2 * t2; 
          c3 += t3 * t3; 
          c4 += t4 * t4; 
          c5 += t5 * t5; 
          c6 += t6 * t6; 
          c7 += t7 * t7; 
#endif
         // too much temp usage, moving of values
#if 1
          c0 += array[i] * array[i];
          c1 += array[i+1] * array[i+1];
          c2 += array[i+2] * array[i+2];
          c3 += array[i+3] * array[i+3];
          c4 += array[i+4] * array[i+4];
          c5 += array[i+5] * array[i+5];
          c6 += array[i+6] * array[i+6];
          c7 += array[i+7] * array[i+7];
          c8 += array[i+8] * array[i+8];
          c9 += array[i+9] * array[i+9];
          c10 += array[i+10] * array[i+10];
          c11 += array[i+11] * array[i+11];
          c12 += array[i+12] * array[i+12];
          c13 += array[i+13] * array[i+13];
          c14 += array[i+14] * array[i+14];
//          c15 += array[i+15] * array[i+15];
#endif
      }

   cctime_t volatile stop_time = cc_get_seconds(clk_freq);
   double run_time_s = ((double) (stop_time - start_time)); 

   g_num_flop[VMAG_IDX] = 2 * array_sz_el;



   // prevent compiler from removing computation...
   // although the receiver must put placebo into a volatile variable as well!
   
   *placebo = (double) c0 
         + (double) c1 
         + (double) c2 
         + (double) c3 
         + (double) c4 
         + (double) c5 
         + (double) c6 
         + (double) c7
         + (double) c8
         + (double) c9
         + (double) c10
         + (double) c11
         + (double) c12
         + (double) c13
         + (double) c14
         ;
   
   return run_time_s;
 }
 

/***********************************/
/*              Main               */
/***********************************/
 
int main(int argc, char* argv[])
{
#ifdef DEBUG  
   printf("\nBegin Test\n");
#endif

   if (argc != 2) 
   {
      //fprintf(stderr, "argc=%d\n", argc);
      fprintf(stderr, "\n[Usage]: peakflops <processor-name>\n\n");
      proc_string = "unknown";
   } else if (argc == 2)
   {
      proc_string = argv[1];
   } 



   // this volatile ret_val is crucial, otherwise the entire run-loop 
   // gets optimized away! :(
   double placebo;

   // Test out Peak Add/Mul Double performance when in a 1:1 ratio
   run_time_s[ADDMUL_IDX] = TestAddMul_1t1(&placebo);  
   double volatile placebo2 = placebo;
   
   // Test out Peak Add Double performance 
   run_time_s[ADD_IDX] = TestAdd(&placebo);  
   placebo2 = placebo;
    
   // Test out Peak Simd Mul/Add Double performance (1:1 ratio)
   run_time_s[SIMDADDMUL_IDX] = TestSimdAddMul_1t1(&placebo);
   placebo2 = placebo;                              
   
   // Test out Peak Simd Add Double performance 
   run_time_s[SIMDADD_IDX] = TestSimdAdd(&placebo);
   placebo2 = placebo;
    
   // Test out Peak Simd Add Double performance 
   run_time_s[VMAG_IDX] = TestVMagnitude(2040, &placebo);
   placebo2 = placebo;
                                                    
 
#ifdef PRINT_SCRIPT_FRIENDLY
   fprintf(stdout, "Proc:[%s],App:[peakflops],RunTime:[%g] (sec), Time:[%g] (ns/iter), MFlop:[%g], MFlops:[%g], PeakDP:[%g],WoutSIMD:[%g], addd:[%g] SimdAdd:[%g],VMagnitude:[%g], NumIterations:[%u]\n",
      proc_string,
      run_time_s[ADDMUL_IDX],
      ((double) run_time_s[ADDMUL_IDX] * 1.0e9 / (double) g_performed_iterations),

      g_num_flop[ADDMUL_IDX] *1e-6,
      ((double) g_num_flop[ADDMUL_IDX]) / ((double) run_time_s[ADDMUL_IDX] * 1.0e6),
//      ((double) g_num_flop[ADDMUL_IDX]) / ((double) run_time_s[ADDMUL_IDX] * 1.0e6)*2.0,
      ((double) g_num_flop[SIMDADDMUL_IDX]) / ((double) run_time_s[SIMDADDMUL_IDX] * 1.0e6),
      ((double) g_num_flop[ADDMUL_IDX]) / ((double) run_time_s[ADDMUL_IDX] * 1.0e6),
      
      ((double) g_num_flop[ADD_IDX]) / ((double) run_time_s[ADD_IDX] * 1.0e6),
      ((double) g_num_flop[SIMDADD_IDX]) / ((double) run_time_s[SIMDADD_IDX] * 1.0e6),
      ((double) g_num_flop[VMAG_IDX]) / ((double) run_time_s[VMAG_IDX] * 1.0e6),
      
	   g_performed_iterations
      );
#endif

#ifdef DEBUG
  fprintf(stderr, "Done. Exiting...\n\n");
#endif

  return 0;
}   

