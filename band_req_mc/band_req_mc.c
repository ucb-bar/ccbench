////////////////////////////////////////////////////////////////////////////////
// BandWidth Request Multicore: (Cache Coherence Micro-Kernel)
// Author: Christopher Celio
// Date  : 2011 May 15
//
// TODO: run for set amount of time, 

#define _XOPEN_SOURCE 600

//#define DEBUG 
//#define PRINT_ARRAY
#define PRINT_SCRIPT_FRIENDLY

// stride in # of elements, most processors have a 64byte cache line, so 16 elements
// would stride across cache lines.
#define CACHELINE_SZ_IN_BYTES (256)
//#define CACHELINE_SZ_IN_BYTES (4)
#define CACHELINE_SZ (CACHELINE_SZ_IN_BYTES/sizeof(uint32_t))

#define PAGE_SZ (4096)
//#define PAGE_SZ (16)
  
//force benchmark to run for some minimum wall-clock time
// advantages: hopefully smoothes out noise of tests that run too quickly
// disvantages: adds additional code to critical loop (empirically unnoticable)
#define USE_MIN_TIME
//min time is in (seconds) 
#define MIN_TIME (1.0)
 

#include "barrier.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <cctimer.h>
#include <cclfsr.h>
#include <math.h>
 
#ifdef TARGET_TILERA
// Tilera junk that tells malloc to use L2s as a distributed L3
#include <malloc.h>  
MALLOC_USE_HASH(1);
#endif


#define NUM_THREADS (16)

pthread_t threads[NUM_THREADS];
pthread_barrier_t barrier;

// Global Variables
uint32_t  g_num_threads;
uint32_t  g_num_requests;
uint32_t  g_num_elements;  // number of elements in array
uint32_t  g_num_iterations;
uint32_t  g_num_elements_initial;
uint32_t  g_performed_iterations[NUM_THREADS];

//track time for each thread
double   volatile run_time_ns     [NUM_THREADS];
double   volatile run_time_us     [NUM_THREADS];
double   volatile run_time_ms     [NUM_THREADS];
double   volatile run_time_s      [NUM_THREADS];
uint32_t volatile num_recorded_iterations[NUM_THREADS];


// Function Declarations
uint32_t initializeGlobalArrays(uint32_t* arr_n_ptr, uint32_t num_elements, uint32_t stride, uint32_t offset);
uint32_t threadMain(void* thread_id);
uint32_t printArray(uint32_t iter, uint32_t *arr_ptr, uint32_t num_elements, uint32_t stride, uint32_t offset);
uint32_t verifyArray(uint32_t *arr_ptr, uint32_t num_elements, uint32_t stride, uint32_t offset);


int main(int argc, char* argv[])
{
#ifdef DEBUG  
   printf("\nBegin Test\n");
#endif

   if (argc != 5) 
   {
      fprintf(stderr, "argc=%d\n", argc);
      fprintf(stderr, "\n[Usage]: band_req_mc <Number of Threads> <Number of Mem Requests Issued in Parallel> <Number of Array Elements (Total)> <Number of Iterations>\n\n");
      return 1;
   }

   g_num_threads    = atoi(argv[1]);
   g_num_requests   = atoi(argv[2]);
   g_num_elements   = atoi(argv[3]);
   g_num_iterations = atoi(argv[4]);

#ifdef DEBUG
   fprintf(stderr, "Number of Parallel Requests = %d\n",  g_num_requests);
   fprintf(stderr, "Size of the array     = %d\n",  g_num_elements);
   fprintf(stderr, "Number of Iterations  = %d\n\n",g_num_iterations);
#endif


   uint32_t err_no = 0;

   pthread_barrier_init(&barrier, NULL, g_num_threads);

   uint32_t tid[NUM_THREADS];
   for (int i=0; i < NUM_THREADS; i++)
   {
      tid[i] = i;
   }


   // Create Threads
   for (int i=0; i < g_num_threads; i++)
   {
      err_no = pthread_create(&threads[i], NULL, (void*) threadMain, (void*) &tid[i]);
      if (err_no != 0)
         printf("** ERROR: pthread_create failed with Err_No: %d\n", err_no);
   }

   
   // Join Threads
   for (int i=0; i < g_num_threads; i++)
      pthread_join(threads[i], NULL);

                
#ifdef DEBUG
   fprintf(stderr, "Total_Time (s)             : %g\n", run_time_s[0]);
   fprintf(stderr, "Total_Time (ms)            : %g\n", run_time_ms[0]);
   fprintf(stderr, "Total_Time (us)            : %g\n", run_time_us[0]);
   fprintf(stderr, "Total_Time (ns)            : %g\n", run_time_ns[0]);
#endif
          


#ifdef PRINT_SCRIPT_FRIENDLY
//   fprintf(stdout, "App:[band_req_mc],NumThreads:[%d], NumRequests:[%d],Size_Req:[%d],AppSize:[%d],Time:[%g], TimeUnits:[Time Per Request (ns)], Bandwidth:[%g], BandwidthUnits:[Bandwidth (Req/s)],NumIterations:[%u]",
//      g_num_threads,
//      g_num_requests,
//      g_num_elements_initial,
//      g_num_elements,
//      (double) ((double) run_time_ns[0] / (double) g_num_iterations / (double) g_num_requests),
//      ((double) g_num_requests * (double) g_num_iterations / (double) run_time_s[0]),
//	   g_num_iterations
//      );
   
   uint64_t total_serviced_requests = 0; 
   for (uint32_t i = 0; i < g_num_threads; i++)
   {
//      num_recorded_iterations[i] = g_num_iterations;
      num_recorded_iterations[i] = g_performed_iterations[i];
      total_serviced_requests += num_recorded_iterations[i]; 
   }

   //measure aggregate bandwidth using the slowest thread... they all started at the same time, but didn't finish at the same time (no barrieR)
   double longest_run_time_s = 0.0;
   for (uint32_t i=0; i < g_num_threads; i++)
   {
//      printf("run_time_s[%d] = %g\n", i, run_time_s[i]);
      if (longest_run_time_s < run_time_s[i])
         longest_run_time_s = run_time_s[i];
   }
   
//   double aggregate_bandwidth = total_serviced_requests / run_time_s[0];
//   printf("longest_run_time_s = %g\n", longest_run_time_s);
//   double aggregate_bandwidth = total_serviced_requests / longest_run_time_s;
   double aggregate_bandwidth = g_num_requests * total_serviced_requests / longest_run_time_s;
   
   for (uint32_t i = 0; i < g_num_threads; i++)
   {
      fprintf(stdout, "App:[band_req_mc],NumThreads:[%d], NumRequests:[%d],Size_Req:[%d],AppSize:[%d],ThreadID:[%d], Time:[%g], TimeUnits:[Time Per Request (ns)], Bandwidth:[%g], BandwidthUnits:[Bandwidth (Req/s)], AggregateBandwidth:[%g], NumIterations:[%u]\n",
         g_num_threads,
         g_num_requests,
         g_num_elements_initial,
         g_num_elements,
         i, 
//         (double) ((double) run_time_ns[i] / (double) g_num_iterations / (double) g_num_requests),
//         ((double) g_num_requests * (double) g_num_iterations / (double) run_time_s[i]),
         (double) ((double) run_time_ns[i] / (double) g_performed_iterations[i] / (double) g_num_requests),
         ((double) g_num_requests * (double) g_performed_iterations[i] / (double) run_time_s[i]),
         aggregate_bandwidth,
//         g_num_iterations
         g_performed_iterations[i]
         );
   }
                         
#endif

#ifdef DEBUG
  fprintf(stderr, "Done. Exiting...\n\n");
#endif

  return 0;
}


uint32_t threadWork(const uint32_t tid, uint32_t* arr_n_ptr)
{
   // clk_freq irrelevant 
   uint32_t const clk_freq = 0;
   cctime_t volatile start_time;
   const uint32_t num_requests = g_num_requests;
   g_performed_iterations[tid] = g_num_iterations;

   uint32_t *idx = malloc(num_requests * sizeof(uint32_t));

//   printf("ThreadWork, Tid = %d\n", tid);
  
   // TODO put each element *somewhere* in its array (so we aren't aliasing on cachelines?)
   for (int i=0; i < num_requests; i++)
   {
      idx[i] = i * (g_num_elements / num_requests);
   }

   pthread_barrier_wait(&barrier);
   start_time = cc_get_seconds(clk_freq);
   cctime_t volatile estimated_end_time = start_time + MIN_TIME;

   
   // TODO standardize on wall clock time? let each thread tack # of successful requests
   // hopefully better account for skew
   // TODO unroll by hand (don't use array)
   // register p0 = *p0; register p1 = *p1;
   for (uint32_t k = 0; k < g_num_iterations; k++)
   {
      for (uint32_t i=0; i < num_requests; i++)
      {
         idx[i]  = arr_n_ptr[idx[i]];
      }
   }

   while (cc_get_seconds(clk_freq) < estimated_end_time)
   {
      g_performed_iterations[tid] += g_num_iterations;
      for (uint32_t k = 0; k < g_num_iterations; k++)
      {
         for (uint32_t i=0; i < num_requests; i++)
         {
            idx[i]  = arr_n_ptr[idx[i]];
         }
      }
   }



   //let each thread tracks its own time (so no barrier here)
   cctime_t volatile stop_time = cc_get_seconds(clk_freq);
     
   run_time_s [tid] = ((double) (stop_time - start_time)); 
   run_time_ns[tid] = run_time_s[tid] * 1.0E9;
   run_time_us[tid] = run_time_s[tid] * 1.0E6;
   run_time_ms[tid] = run_time_s[tid] * 1.0E3;

//   printf("Tid: %d, stop_time: %g    start_time: %g,   run_time_s: %g\n",
//      tid, 
//      stop_time,
//      start_time,
//      run_time_s[tid]);

   // prevent compiler from removing ptr chasing...
   // although the receiver must put idx into a volatile variable as well!
   uint32_t sum = 0;
   for (int i=0; i < num_requests; i++)
      sum += idx[i];

   return sum; 
}

//initialize perthread arrays
//uint32_t threadMain(const uint32_t num_requests)
uint32_t threadMain(void* thread_id)
{
//   uint32_t g_num_elements_initial = g_num_elements;
   g_num_elements_initial = g_num_elements;
   uint32_t tid = (int) *(int*) thread_id;

   // Accesses should hit cache-line addresses, to eliminate spatial locality
   // and force more misses. Therefore, make the array size a multiple of the
   // stride; round up we have to deal with this also working once we split
   // array into smaller pieces
   if ((g_num_elements / g_num_requests)  % CACHELINE_SZ != 0 || g_num_elements < g_num_requests)
   {
      uint32_t num_elements_per_req = g_num_elements / g_num_requests; 
      num_elements_per_req = num_elements_per_req + ((CACHELINE_SZ) - (num_elements_per_req % CACHELINE_SZ));
      g_num_elements = g_num_requests * num_elements_per_req;
   }
      
#ifdef DEBUG
   fprintf(stderr, "adjusted size of array = %d\n", g_num_elements);
#endif

   uint32_t* arr_n_ptr = (uint32_t *) malloc((g_num_elements) * sizeof(uint32_t));

   for (uint32_t i = 0; i < g_num_elements; i++)
      arr_n_ptr[i] = 1337;

   uint32_t stride = CACHELINE_SZ;
   
   
   // Provide each request its own "array" to pointer chase on This prevents
   // the processor from consolidating request streams The fact that we are
   // using a single array to hold all of this is a bit too "clever", but it
   // saves cycles in the critical loop from figuring out which array to use.
   for (int i=0; i < g_num_requests; i++)
   {
      uint32_t num_elements_per_req = g_num_elements / g_num_requests;

      //printf("tid=%d, i=%d of %d,  num_elements = %d, num_el_per_req= %d  Size Per Req= %d bytes cacheline_sz = %d, (num_el_per_req mod cl_sz=%d)   &(array[%d])\n\n",
      //   tid,
      //   i,
      //   g_num_requests,
      //   g_num_elements,
      //   num_elements_per_req,
      //   num_elements_per_req * 4,
      //   CACHELINE_SZ,
      //   num_elements_per_req % CACHELINE_SZ,
      //   i * num_elements_per_req
      //   );
     
      initializeGlobalArrays( arr_n_ptr, 
                              num_elements_per_req,
                              stride,
                              i * num_elements_per_req);
   }
   

//   printf("tid=%d SUCCESS\n", tid);

   // this volatile ret_val is crucial, otherwise the entire run-loop 
   // gets optimized away! :(
//   uint32_t volatile ret_val = threadWork(g_num_requests);  
   uint32_t volatile ret_val = threadWork(tid, arr_n_ptr);  

   return ret_val;
}








        
//generate a random linked list of a PAGE_SZ, starting at address arr_n_ptr[page_offset]
//the calling function is in charge of stitching the page together with the other pages
//num_accesses is the number of pointers we are going to add (not the number of
//elements spanned by the page)
//returns the index to the last index (to help the calling function stitch pages together)
uint32_t initializePage(uint32_t* arr_n_ptr, uint32_t page_offset, uint32_t num_accesses, uint32_t stride)
{
#ifdef DEBUG
   printf("\n--(offset = %d) Generating Page-- num_el=%d, pageSz=%d, strd=%d\n\n",
      page_offset, num_accesses, num_accesses*stride*sizeof(int32_t), stride);
#endif

   cc_lfsr_t lfsr;
   uint32_t lfsr_init_val = 1; //TODO provide different streams different starting positions?
   uint32_t lfsr_width = (log(num_accesses) / log(2)); 

   uint32_t max_accesses = (0x1 << lfsr_width);

   // special case to handle non-powers of 2 num_accesses
   if (max_accesses < num_accesses)
      lfsr_width++;


//   uint32_t idx;
   uint32_t curr_idx, next_idx;

   curr_idx = page_offset;

   // special cases to handle the VERY small pages (too small to use a LFSR)
   if (lfsr_width < 2)
   {
      // lfsr width too small, forget about randomizing this page...
      for (int i=0; i < num_accesses-1; i++)
      {
         arr_n_ptr[curr_idx] = curr_idx + stride;
         curr_idx += stride; 
      }
      
      arr_n_ptr[curr_idx] = page_offset;
      return curr_idx;
   }

   
   cc_lfsr_init(&lfsr, lfsr_init_val, lfsr_width);


   // "-1" because we do the last part separately (lfsr's don't generate 0s)
   for (int i=0; i < num_accesses-1; i++)
   {
      next_idx = lfsr.value * stride + page_offset; 
      
      arr_n_ptr[curr_idx] = next_idx;

#ifdef DEBUG
      printf("   array[%4d] = %4d,   lfsr.value = %4d, i = %4d \n", 
                  curr_idx, arr_n_ptr[curr_idx], lfsr.value, i); 
#endif

      curr_idx = next_idx;
      lfsr.value = cc_lfsr_next(&lfsr);

      //handle non powers of 2 num_accesses
      while (lfsr.value > (num_accesses-1))
      {
         lfsr.value = cc_lfsr_next(&lfsr);
      }
   }

   //handle last index, wrap back to the start of the page
   //let calling function stitch to next pages...
   arr_n_ptr[curr_idx] = page_offset;

   return curr_idx;
}
 
 
uint32_t initializeGlobalArrays(uint32_t* arr_n_ptr, uint32_t num_elements, uint32_t stride, uint32_t arr_offset)
{

#ifdef DEBUG
   printf("\nInitializing Array at 0x%x, num_elements= %d, stride= %d, offset= %d, num_cl = %d\n\n",
      arr_n_ptr, num_elements, stride, arr_offset, num_elements / stride); 
#endif

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
      // points back on itself
      arr_n_ptr[0 + arr_offset] = 0 + arr_offset;
      return 0;
   }
      

//   uint32_t num_elements_per_page = PAGE_SZ/sizeof(uint32_t)/stride;
   uint32_t num_elements_per_page = PAGE_SZ/sizeof(uint32_t);
   uint32_t num_accesses_per_page = PAGE_SZ/sizeof(uint32_t)/stride;

     
   uint32_t last_idx = 0;

   // two-level randomization... (this is the outer level for-loop)
   // for each page...
   for(int i=0; i < num_elements; i+=num_elements_per_page)
   {
      uint32_t page_offset = i + arr_offset;

#ifdef DEBUG
      printf("\nStarting New Page: i=%d, page offset = %d, num_elements = %d, num_elements_per_page = %d, num_accesses_per_page = %d\n", 
         i, i, num_elements, num_elements_per_page, num_accesses_per_page);
#endif
         
       
      if ((int32_t) i >= (int32_t) (num_elements - num_elements_per_page)  //is last page?
         && ((num_elements - i) % num_elements_per_page != 0))  //and last page is partial
      {
         num_elements_per_page = num_elements - i; 
         num_accesses_per_page = num_elements_per_page / stride;
#ifdef DEBUG
         printf("last page (partial), new num_elements_per_page = %d\n", 
            num_elements_per_page);
#endif
      }



      last_idx = initializePage(arr_n_ptr, page_offset, num_accesses_per_page, stride);
      // tie this page to the next page...
      arr_n_ptr[last_idx] =  page_offset + (PAGE_SZ/sizeof(uint32_t)); 
#ifdef DEBUG
      printf("  *array[%4d] = %4d, \n", last_idx, arr_n_ptr[last_idx]);
#endif
   }


      //handle the last page ...
      //wrap the array back to the start
      if (last_idx >= num_elements*g_num_requests)
         printf("THIS SHOULD BE CRASHING, last_idx = %u >= num_elements = %u\n",
            last_idx, num_elements*g_num_requests);

      arr_n_ptr[last_idx] = 0 + arr_offset;
#ifdef DEBUG
      printf(" **array[%4d] = %4d (looping back to the start: arr_offset=%d \n", 
         last_idx, arr_n_ptr[last_idx], arr_offset);
#endif

#ifdef PRINT_ARRAY
   printArray(0, arr_n_ptr, num_elements, stride, arr_offset);
#endif
   verifyArray(arr_n_ptr, num_elements, stride, arr_offset);

   return 0;
}
 

uint32_t printArray(uint32_t iter, uint32_t *arr_ptr, uint32_t num_elements, uint32_t stride, uint32_t offset)
{
   fprintf(stderr, "Chasing through Array (run thru for 2x*num_el) after iteration: %d\n", iter);
   uint32_t idx = 0 + offset;
   for (uint32_t i = 0; i < 2*num_elements; i++)
   {
      if (i==num_elements)
         fprintf(stderr, "\n\n");
      fprintf(stderr, "%d, ", arr_ptr[idx]);
      idx = arr_ptr[idx];
   }
   fprintf(stderr, "\n");
   
   fprintf(stderr, "arr_n_ptr: ");
   for (uint32_t i = 0; i < num_elements; i++)
   {
      fprintf(stderr, "%d, ", arr_ptr[i]);
   }
   fprintf(stderr, "\n");
   
   return 0;
}


uint32_t verifyArray(uint32_t *arr_ptr, uint32_t num_elements, uint32_t stride, uint32_t offset)
{
#ifdef DEBUG
   printf("Verifying Array at 0x%x, num_elements=%d, offset=%d\n",
      arr_ptr, num_elements, offset);
#endif

   uint32_t idx = 0 + offset;
   uint32_t counter = 0;

   uint32_t first_idx = 0 + offset;
   uint32_t finished = 0;

   uint32_t* verify_array;
   verify_array = (uint32_t *) malloc((num_elements) * sizeof(uint32_t));
   
   while (!finished)
   {
//      printf(" verifying=== idx = %d, offset = %d, num_elements = %d\n", idx, offset, num_elements);
      if (idx < offset) {
         printf("ERROR: verification failed, pointer chased to out-of-bounds element, idx = %u, offset = %u, verify[%u]\n",
            idx, offset, idx - offset);
         exit(1);
      }

      verify_array[idx - offset] = verify_array[idx - offset] + 1;
      counter++;
      idx = arr_ptr[idx];

      if(counter >= num_elements || idx == first_idx)
         finished = 1;
   }

   uint32_t error = 0;

   for (uint32_t i = 0; i < num_elements; i+=CACHELINE_SZ)
   {
      if(verify_array[i] != 1)  
      {
         fprintf(stderr, "Error at Element [%d], accessed %d times\n", 
            i + offset,
            verify_array[i]
            );
         error = 1;
      }
   }

   if (counter==(num_elements/CACHELINE_SZ) && !error) {
#ifdef DEBUG
      fprintf(stderr,"Array verified\n\n");
#else
      int x=0; //here for compiler reasons
#endif
   } 
   else 
   {
      fprintf(stderr,"Error: Array size:%d, Stride size:%lu, Loops in:%u\n\n",
//      fprintf(stderr,"Error: Array size:d, Stride size:%lu, Loops in:d\n\n",
         num_elements,
         CACHELINE_SZ,
         counter);
   }

   return error | (counter==(num_elements/CACHELINE_SZ));
}

