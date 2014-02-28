////////////////////////////////////////////////////////////////////////////////
// Incluexclu (figure out if a higher level cache is inclusive or exclusive)
// Author: Christopher Celio
// Date  : 2011 May 19
//
// Runs two threads (on different cores).  One thread (Big) runs out of
// off-chip memory (forcing lots of capcity misses in the LLC). The second
// thread (Small) runs on an array of a fixed size. If the LLC is exclusive,
// this second thread will run fast.  If the LLC is inclusive, the second
// thread will have lots of misses due to the second thread forcing out its
// working set.
//
// TODO: restrict the addresses being pinged to force more interaction between the two threads

#define _XOPEN_SOURCE 600

//#define DEBUG 
//#define PRINT_ARRAY
#define PRINT_SCRIPT_FRIENDLY
 

#define PAGE_SZ (4096*8)
//#define PAGE_SZ (4096)
#define WAY_SZ (4096*2)
//int posix_memalign(void **memptr, size_t alignment, size_t size);

 
// stride in # of elements, core 2 duo has a 64byte cache line, so 16 elements
// would stride across cache lines!
#define CACHELINE_SZ_IN_BYTES (64)
#define CACHELINE_SZ (CACHELINE_SZ_IN_BYTES/4)
//#define NUM_WAYS (8)

#define MAX_THREADS (2)
#define BIG_NUM_REQUESTS (32)
//#define BIG_NUM_REQUESTS (1)
 
// 16 MB, should run out of off-chip memory
#define BIG_ARRAY_SZ_IN_BYTES (16777216)
//#define BIG_ARRAY_SZ_IN_BYTES (4096*PAGE_SZ)
         
#include "barrier.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <cctimer.h>
#include <cclfsr.h>
#include <math.h>



#ifndef __APPLE__
#include <sched.h>
#define THREAD1_MASK 0
#define THREAD2_MASK 2
#define THREAD3_MASK 4
#define THREAD4_MASK 8
cpu_set_t cpu[MAX_THREADS];
#endif



// smaller array
uint32_t  g_num_threads;  
uint32_t  g_num_elements;  // number of elements in Small array
uint32_t  g_num_iterations;
uint32_t  g_stride;
uint32_t  g_run_type; //do we or do we not alias addresses together (serves as a control)

volatile uint32_t g_not_finished; //semaphore to tell Big thread to keep running

pthread_barrier_t       barrier;
pthread_barrier_attr_t  barrier_attributes;
pthread_attr_t          thread_attributes[MAX_THREADS];
 
double volatile run_time_ns;
double volatile run_time_us;
double volatile run_time_ms;
double volatile run_time_s;


// Function Declarations
uint32_t initializeGlobalArrays(uint32_t* arr_n_ptr, uint32_t num_elements, uint32_t stride, uint32_t tid);
uint32_t threadMainBig();
uint32_t threadMainSmall();
uint32_t printArray(uint32_t iter, uint32_t* arr_n_ptr, uint32_t num_elements, uint32_t stride);
uint32_t verifyArray(uint32_t *arr_ptr, uint32_t num_elements, uint32_t stride, uint32_t tid);


// OS X doesn't provide memory alignment, so for compatibility sake let's write our own
void* aligned_malloc( uint32_t align, size_t size )
{
#ifdef DEBUG
   printf("aligned_malloc: size = %d, align = %d\n", (uint32_t) size, (uint32_t) align);
#endif

   //allocate more than we need
   void *mem = malloc( size + (align-1) + sizeof(void*) );
   if (!mem) return NULL;

   char *amem = ((char*)mem) + sizeof(void*);
   amem += align - ((uint64_t)amem & (align - 1));

   ((void**)amem)[-1] = mem;
   return amem;
}

void aligned_free( void *mem )
{
   free( ((void**)mem)[-1] );
}



int main(int argc, char* argv[])
{
#ifdef DEBUG  
   printf("\nBegin Test\n");
#endif

   if (argc != 4) 
   {
      fprintf(stderr, "argc=%d\n", argc);
      fprintf(stderr, "\n[Usage]: incluexclu > <Number of Threads> <Number of Array Elements for Small Thread> <Number of Iterations> \n\n");
//      return 1;
      g_num_threads    = 1;
      g_num_elements   = 1024;
      g_num_iterations = 100;
   } 
   else 
   {
      g_num_threads    = atoi(argv[1]);
      g_num_elements   = atoi(argv[2]);
      g_num_iterations = atoi(argv[3]);
   }

//   if (g_num_threads > MAX_THREADS)
//   {
//      fprintf(stderr, "Too many threads requested.\n");
//      return -1;
//   }

#ifdef DEBUG
   fprintf(stderr, "Number of Threads     = %d\n", g_num_threads);
   fprintf(stderr, "Size of the array     = %d\n", g_num_elements);
   fprintf(stderr, "Number of Iterations  = %d\n", g_num_iterations);
#endif

   // eliminate spatial locality and force more misses.
//   g_stride = CACHELINE_SZ_IN_BYTES / 4;
//   g_stride = WAY_SZ / 4;
   g_stride = (WAY_SZ / 4);

   // make the array size a multiple of the stride; round up
   if (g_num_elements % g_stride != 0)
      g_num_elements = g_num_elements + ((g_stride) - (g_num_elements % g_stride));
 
#ifdef DEBUG
   fprintf(stderr, "Adjusted Size         = %d (%lu bytes)\n\n",  
      g_num_elements, g_num_elements*sizeof(uint32_t));
#endif
 
  
   // prevent compiler from doing wierd optimizations on our run loops
   uint32_t volatile ret_val0;
   uint32_t volatile ret_val1;

 
   pthread_t threads[MAX_THREADS];

#ifndef __APPLE__
//TODO programatically build this up
   CPU_ZERO(&cpu[0]);
   CPU_SET(THREAD0_MASK, &cpu[0]);
   CPU_ZERO(&cpu[1]);
   CPU_SET(THREAD1_MASK, &cpu[1]);
   CPU_ZERO(&cpu[2]);
   CPU_SET(THREAD2_MASK, &cpu[2]);
   CPU_ZERO(&cpu[3]);
   CPU_SET(THREAD3_MASK, &cpu[3]);
#endif

   if (g_num_threads > 2) 
   {
      g_run_type = 1;
      g_num_threads = 2;
   }
   else
      g_run_type = 0;


   for (int i=0; i < g_num_threads; i++)
      pthread_attr_init(&thread_attributes[i]);

#ifndef __APPLE__
   for (int i=0; i < g_num_threads; i++)
      pthread_attr_setaffinity_np(&thread_attributes[i], sizeof(cpu_set_t), &cpu[i]);
#endif
   
   pthread_barrier_attr_init(&barrier_attributes);
   pthread_barrier_init(&barrier, &barrier_attributes, g_num_threads);


   pthread_create(&threads[0], &thread_attributes[0], (void *(*)())threadMainSmall, NULL); 
   
   for (int i=1; i < g_num_threads; i++)
      pthread_create(&threads[i], &thread_attributes[1], (void *(*)())threadMainBig, NULL); 
  

   pthread_join(threads[0], (void*) &ret_val0);
   for (int i=1; i < g_num_threads; i++)
      pthread_join(threads[i], (void*) &ret_val1);

   if (g_run_type == 1)
      g_num_threads = 3;

#ifdef PRINT_SCRIPT_FRIENDLY
   fprintf(stdout, "App:[incluexclu,NumThreads:[%d],AppSize:[%d],Time:[%g], TimeUnits:[Time Per Iteration (ns)],NumIterations:[%u]\n",
      g_num_threads,
      g_num_elements,
      ((double) run_time_ns / (double) g_num_iterations),
	   g_num_iterations
      );
#endif

#ifdef DEBUG
  fprintf(stderr, "Done. Exiting...\n\n");
#endif

  return 0;
}


uint32_t threadMainSmall()
{
   uint32_t tid = 0;

   uint32_t* arr_n_ptr; //array of next pointers

   void** memblock1;
   void** memblock2;

   uint32_t num_elements = g_num_elements;
  
   arr_n_ptr = (uint32_t *) aligned_malloc(PAGE_SZ, (num_elements) * sizeof(uint32_t));

#ifdef DEBUG   
   printf("Allocated memory address:(void **)  0x%x %x\n", 
      (uint32_t) (0xFFFFFFFF & (((uint64_t) arr_n_ptr) >> 32)),
      (uint32_t) (0xFFFFFFFF & (uint64_t) arr_n_ptr));
#endif


   initializeGlobalArrays(arr_n_ptr, num_elements, g_stride, tid);
   
   
   // flag to tell Big thread to keep executing until small thread finishes
   g_not_finished = 1;
   pthread_barrier_wait(&barrier);
   
   // clk_freq irrelevant 
   uint32_t const clk_freq = 0;
   cctime_t volatile start_time = cc_get_seconds(clk_freq);

   volatile uint32_t idx = 0;

   for (uint32_t k = 0; k < g_num_iterations; k++)
   {
      idx = arr_n_ptr[idx];
   }

   cctime_t volatile stop_time = cc_get_seconds(clk_freq);
   g_not_finished = 0;
        
   run_time_s = ((double) (stop_time - start_time)); 
   run_time_ns = run_time_s * 1.0E9;
   run_time_us = run_time_s * 1.0E6;
   run_time_ms = run_time_s * 1.0E3;

#ifdef DEBUG
   fprintf(stderr, "Total_Time (s)             : %g\n", run_time_s);
   fprintf(stderr, "Total_Time (ms)            : %g\n", run_time_ms);
   fprintf(stderr, "Total_Time (us)            : %g\n", run_time_us);
   fprintf(stderr, "Total_Time (ns)            : %g\n", run_time_ns);
#endif

   // prevent compiler from removing ptr chasing...
   // although the receiver must put idx into a volatile variable as well!
   return idx; 
}
 
// run out of off-chip memory to force lots of LLC capcity misses
uint32_t threadMainBig()
{
   uint32_t tid = 1;

   uint32_t* arr_n_ptr; //array of next pointers

   uint32_t num_elements = BIG_ARRAY_SZ_IN_BYTES/sizeof(uint32_t);
  
   // alias addresses with smaller thread?
   if (g_run_type)
      arr_n_ptr = (uint32_t *) aligned_malloc((PAGE_SZ+CACHELINE_SZ_IN_BYTES), (num_elements) * sizeof(uint32_t));
   else
      arr_n_ptr = (uint32_t *) aligned_malloc(PAGE_SZ, (num_elements) * sizeof(uint32_t));
   
   initializeGlobalArrays(arr_n_ptr, num_elements, g_stride, tid);


   pthread_barrier_wait(&barrier);

   uint32_t *idx = malloc(BIG_NUM_REQUESTS * sizeof(uint32_t));
   
   //TODO need to get this right in terms of stride, and exhibit the least locality
   for (int i=0; i < BIG_NUM_REQUESTS; i++)
   {
//      idx[i] = arr_n_ptr[i]; //i * (BIG_ARRAY_SZ_IN_BYTES / 4 / BIG_NUM_REQUESTS);
      idx[i] = arr_n_ptr[i* (BIG_ARRAY_SZ_IN_BYTES / sizeof(uint32_t) / BIG_NUM_REQUESTS)];

      if (idx[i] == 0)
      {
         printf("ERROR: initializes idx[] to a null element, arr_n_ptr[%d]=%d\n",
            i, arr_n_ptr[i]);
      }
   }

 
   while (g_not_finished)
   {
//      printf("Big: notFinished? =%d, idx[0]=%d\n", g_not_finished, idx[0]);
      for (uint32_t i=0; i < BIG_NUM_REQUESTS; i++)
         idx[i]  = arr_n_ptr[idx[i]];
   }

   // prevent compiler from removing ptr chasing...
   // although the receiver must put idx into a volatile variable as well!
   uint32_t sum = 0;
   for (int i=0; i < BIG_NUM_REQUESTS; i++)
      sum += idx[i];
   
   return sum; 
}
 
uint32_t initializeGlobalArrays(uint32_t* arr_n_ptr, uint32_t num_elements, uint32_t stride, uint32_t tid)
{
   //randomize array on cacheline strided boundaries
#ifdef DEBUG
   printf("\n-==== Begin two-level randomization of the chase array ====-\n");
#endif

   // check to see if the array is 1-element long...
   if (num_elements == stride)
   {
#ifdef DEBUG
      printf("\nArray is 1 element long, returning....\n");
#endif
      arr_n_ptr[0] = 0;
      return 0;
   }
      

   uint32_t num_elements_per_page = PAGE_SZ/sizeof(uint32_t)/stride;
#ifdef DEBUG
   printf("\nnum_elements_per_page = %d\n\n", PAGE_SZ/sizeof(uint32_t)/stride);
#endif

   uint32_t init_val = 1;

   cc_lfsr_t lfsr;
      
#ifdef DEBUG
   printf("Page Size=%d kB, Page Size (in Elements) = %d, lfsr.width=%d, num_elements = %d, stride = %d, num_elements/stride = %d  \n", 
      PAGE_SZ, (PAGE_SZ/CACHELINE_SZ_IN_BYTES), (uint32_t) (log(num_elements_per_page) / log(2)), num_elements, stride, num_elements/stride);
#endif
      
   // TODO make a new lfsr for each page, to better handle fractions of a page (better performanace)
   cc_lfsr_init(&lfsr, init_val, (uint32_t) (log((num_elements_per_page)) / log(2)));
     
   // two-level randomization... (this is the outer level for-loop)
   for(int i=0; i < num_elements/stride; i+=num_elements_per_page)
   {
#ifdef DEBUG
      printf("\nStarting New Page: i=%d, page offset = %d, lfsr.value=%d, lfsr.width=%d, Max(innerloop) = %d\n", 
         i, i, lfsr.value, lfsr.width, ((num_elements_per_page) -1));
#endif
         
      uint32_t page_offset = i*stride;

      //to handle randomizing for not-perfect-page-sizes...
      uint32_t curr_num_elements_in_page = num_elements_per_page; 


      // is it a partial page? 
      if ( (int32_t) i >= (int32_t) (num_elements/stride - (num_elements_per_page)) )
      {
#ifdef DEBUG
         printf("\n\n PARTIAL PAGE \n");
#endif
         curr_num_elements_in_page = (num_elements/stride - i) % (num_elements_per_page);
      }

#ifdef DEBUG
      printf("\n ****curr_num_elements_in_page is (%d)\n", curr_num_elements_in_page);
#endif
      
      if (curr_num_elements_in_page == 0) {
#ifdef DEBUG
         printf("\n ****curr_num_elements_in_page was zero, setting back to the full curr_num_elements_in_page?\n");   
#endif
         curr_num_elements_in_page = num_elements_per_page; //in cachelines...
      }
      
      
      
#ifdef DEBUG
      printf("\n   array[%d] = %d,   lfsr.value = %d,  curr_num_elements_in_page = %d\n", i, lfsr.value*stride+page_offset, lfsr.value, curr_num_elements_in_page);
#endif

      if (curr_num_elements_in_page > 1)
         arr_n_ptr[i*stride] = lfsr.value * stride + page_offset;

      // starts at j=1 because we already manually set j=0
      for(int j=1; j < (num_elements_per_page) - 1 && j < curr_num_elements_in_page - 1; j++)
      {
         uint32_t curr = lfsr.value * stride + page_offset;
         
         lfsr.value = cc_lfsr_next(&lfsr);

         //handle randomizing for sections that aren't perfect multiples of the page size
         while(lfsr.value >= curr_num_elements_in_page)
         {
#ifdef DEBUG
            printf("                rerolling a value..... lfsr.value = %d    , curr_num_elements_in_page = %d\n", lfsr.value, curr_num_elements_in_page);
#endif
            lfsr.value = cc_lfsr_next(&lfsr);
         }

         arr_n_ptr[curr] = lfsr.value * stride + page_offset;
         
#ifdef DEBUG
         printf("   array[%d] = %d,   lfsr.value = %d, j = %d, curr_num_elements_in_page = %d\n", 
            curr, lfsr.value*stride+page_offset, lfsr.value, j, curr_num_elements_in_page);
#endif
      }

      // if it's the last round, wrap the element back...
      uint32_t curr = lfsr.value * stride + page_offset;
      lfsr.value = cc_lfsr_next(&lfsr);
      
      if ((int32_t) i >= ((int32_t) (num_elements/stride - (num_elements_per_page))))
      {
         arr_n_ptr[curr] = 0;
         
#ifdef DEBUG
         printf("   array[%d] = %d,   lfsr.value = %d **\n", 
            curr, 0, lfsr.value);
#endif
      }
      else
      {
         //tie this page to the next page 
         arr_n_ptr[curr] = 0 * stride + page_offset + (PAGE_SZ/sizeof(uint32_t)); 
         
#ifdef DEBUG
         printf("   array[%d] = %lu,   lfsr.value = %d *\n", 
            curr, 0*stride+page_offset+(PAGE_SZ/sizeof(uint32_t)), lfsr.value);
#endif
      }
         
   }

#ifdef DEBUG
   printf("\n\n");
#endif

#ifdef PRINT_ARRAY
   printArray(0, arr_n_ptr, num_elements, stride);
#endif

   verifyArray(arr_n_ptr, num_elements, stride, tid);
  
   return 0;


#if 0
   //creats linear linked list
   for (uint32_t i = 0; i < num_elements-1; i++)
   {
      arr_n_ptr[i % num_elements]           = (i+g_stride) % num_elements;
      arr_p_ptr[(i+g_stride) % num_elements]  = i % num_elements;
   }
   
   arr_p_ptr[0] = num_elements - g_stride;


#ifdef PRINT_ARRAY
   printArray(arr_n_ptr, num_elements, 0);
#endif

   verifyArray(arr_n_ptr, num_elements, tid);

  return 0;
#endif
}

uint32_t verifyArray(uint32_t *arr_ptr, uint32_t num_elements, uint32_t stride, uint32_t tid)
{
   uint32_t idx = 0;
   uint32_t counter = 0;

   uint32_t first_idx = 0;
   uint32_t finished = 0;

   uint32_t* verify_array;
   verify_array = (uint32_t *) malloc((num_elements) * sizeof(uint32_t));
   
   while (!finished)
   {
      verify_array[idx] = verify_array[idx] + 1;
      counter++;
      idx = arr_ptr[idx];

      if(counter >= num_elements || idx == first_idx)
         finished = 1;
   }

   uint32_t error = 0;

   for (uint32_t i = 0; i < num_elements; i+=stride)
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

   if (counter==(num_elements/stride) && !error) {
#ifdef DEBUG
      fprintf(stderr,"Array verified\n\n");
#else
      int x=0; //here for compiler reasons
#endif
   } 
   else 
   {
      fprintf(stderr,"Error: Array size: %d, Stride size: %d, Loops in: %d, Should Loop in %d\n\n",
         num_elements,
         stride,
         counter,
         num_elements/stride);
   }

   return error | (counter==(num_elements/stride));
}
 
uint32_t printArray(uint32_t iter, uint32_t* arr_n_ptr, uint32_t num_elements, uint32_t stride)
{
   fprintf(stderr, "Chasing through Array (run thru for 2x*num_el/stride) after iteration: %d\n", iter);
   uint32_t idx = 0;

   //prevent from printing out absurdly large arrays....
   if (num_elements/stride > 16)
      num_elements = stride*16;

   for (uint32_t i = 0; i < 2*num_elements/stride; i++)
   {
      fprintf(stderr, "%d (0x%8.x %8.x), ", 
         arr_n_ptr[idx], 
         ((uint32_t) (0xFFFFFFFF & ((uint64_t) &(arr_n_ptr[idx])) >> 32)) ,
         (uint32_t) (0xFFFFFFFF & ((uint64_t) &(arr_n_ptr[idx]))) );
      idx = arr_n_ptr[idx];
   }
   fprintf(stderr, "\n");
#if 0   
   fprintf(stderr, "arr_n_ptr: ");
   for (uint32_t i = 0; i < num_elements; i++)
   {
      if (i % stride == 0) fprintf(stderr, "\n");
      fprintf(stderr, "%d, ", arr_n_ptr[i]);
   }
#endif
   fprintf(stderr, "\n\n\n");

   return 0;
}
 
