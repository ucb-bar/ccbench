////////////////////////////////////////////////////////////////////////////////
// LFSR Test
// Author: Christopher Celio
// Date  : 2010 Nov 15
//
// This is NOT a benchmark.
//
// This is to test out building m-sequence LFSRs (linear feedback shift
// registers) that produce a string of random numbers that do not repeat.
// Useful for generating arrays of random pointers for defeating prefetchers.
// 

// stride in # of elements, core 2 duo has a 64byte cache line, so 16 elements
// would stride across cache lines!
#define CACHELINE_SZ (64/4)
#define CACHELINE_IN_BYTES_SZ (64)

#define PAGE_SZ (4096)

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <cctimer.h>
#include <cclfsr.h>

// Global Variables
uint32_t* g_arr_n_ptr;     //next pointers
uint32_t* g_arr_p_ptr;     //prev pointers
uint32_t  g_num_elements;  // number of elements in array
uint32_t  g_num_iterations;

int g_stride;

// Function Declarations
uint32_t initializeGlobalArrays();
uint32_t printArray(uint32_t iter);
uint32_t verifyArray();

uint32_t getBit(uint32_t current, uint32_t pos);
uint32_t getNextValue(uint32_t current);

int main(int argc, char* argv[])
{
   printf("\nBegin Test\n");

   if (argc != 3) 
   {
      fprintf(stderr, "argc=%d\n", argc);
      fprintf(stderr, "\n[Usage]: lfsr <Number of Array Elements (Total)> <Number of Iterations> <RunType> \n\n");
      return 1;
   }

   g_num_elements   = atoi(argv[1]);
   g_num_iterations = atoi(argv[2]);

   fprintf(stderr, "Size of the array     = %d\n",  g_num_elements);
   fprintf(stderr, "Number of Iterations  = %d\n\n",g_num_iterations);


   g_stride = 1;

   // make the array size a multiple of the stride; round up
   if (g_num_elements % g_stride != 0)
      g_num_elements = g_num_elements + ((g_stride) - (g_num_elements % g_stride));

//   initializeGlobalArrays();

   fprintf(stderr, "Adjusted Size of the array = %d\n",  g_num_elements);
   g_arr_n_ptr = (uint32_t*) malloc(g_num_elements * sizeof(uint32_t));

   uint32_t init_val = 1;

   cc_lfsr_t lfsr;
   cc_lfsr_init(&lfsr, init_val, log(g_num_elements) / log(2));


   printf(" -==== (%d-bit) LFSR ====-\n", lfsr.width);

   // zero never gets generated, so give it a push
   g_arr_n_ptr[0] = lfsr.value;

   // remember to see if we pick out neighboring values
   
   for(int i=0; i < g_num_iterations; i++)
   {
      printf("   (%d-bit) LFSR = %d\n", lfsr.width, lfsr.value);
      uint32_t last_val = lfsr.value; 

      lfsr.value = cc_lfsr_next(&lfsr);
      g_arr_n_ptr[last_val] = lfsr.value;

      if ( last_val == lfsr.value - 1
         || last_val == lfsr.value + 1)
         printf("              <= neighbor picked!\n");
   }


   verifyArray();

   fprintf(stdout, "App:[lfsr],AppSize:[%d],NumIterations:[%u]\n",
      g_num_elements,
	   g_num_iterations
      );

  fprintf(stderr, "Done. Exiting...\n\n");

  return 0;
}


uint32_t printArray(uint32_t iter)
{
   fprintf(stderr, "Chasing through Array (run thru for 2x*num_el) after iteration: %d\n", iter);
   uint32_t idx = 0;
   for (uint32_t i = 0; i < 2*g_num_elements; i++)
   {
      fprintf(stderr, "%d, ", g_arr_n_ptr[idx]);
      idx = g_arr_n_ptr[idx];
   }
   fprintf(stderr, "\n");
   
   fprintf(stderr, "arr_n_ptr: ");
   for (uint32_t i = 0; i < g_num_elements; i++)
   {
      fprintf(stderr, "%d, ", g_arr_n_ptr[i]);
   }
   fprintf(stderr, "\n");
   
   fprintf(stderr, "arr_p_ptr (twice thru): ");
   for (uint32_t i = 0; i < 2*g_num_elements; i++)
   {
      idx = i % g_num_elements;
      fprintf(stderr, "%d, ", g_arr_p_ptr[idx]);
   }
   fprintf(stderr, "\nchase through arr_p_ptr (2x*num_el): ");
   idx = 0;
   for (uint32_t i = 0; i < 2*g_num_elements; i++)
   {
      fprintf(stderr, "%d, ", g_arr_p_ptr[idx]);
      idx = g_arr_n_ptr[idx];
   }
   fprintf(stderr, "\n\n");

   return 0;
}


uint32_t verifyArray()
{
   uint32_t idx = 0;
   uint32_t counter = 0;

   uint32_t first_idx = 0;
   uint32_t finished = 0;

   uint32_t* verify_array;
   verify_array = (uint32_t *) malloc((g_num_elements) * sizeof(uint32_t));
   
   while (!finished)
   {
      verify_array[idx] = verify_array[idx] + 1;
      counter++;
      idx = g_arr_n_ptr[idx];

      if(counter >= g_num_elements || idx == first_idx)
         finished = 1;
   }

   uint32_t error = 0;

   for (uint32_t i = 0; i < g_num_elements; i+=g_stride)
   {
      if(verify_array[i] != 1)  
      {
         fprintf(stderr, "Error at Element [%d], accessed %d times\n", 
            i,
            verify_array[i]
            );
         error = 1;
      }
   }

   if (counter==(g_num_elements/g_stride) && !error) {
      fprintf(stderr,"Array verified\n\n");
   } 
   else 
   {
      fprintf(stderr,"Error: Array size:%d, Stride size:%d, Loops in:%d\n\n",
         g_num_elements,
         g_stride,
         counter);
   }

   return error | (counter==(g_num_elements/g_stride));
}

