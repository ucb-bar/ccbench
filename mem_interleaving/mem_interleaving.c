////////////////////////////////////////////////////////////////////////////////
// Memory Interleaving: (Cache Coherence Micro-Kernel)
// Author: Christopher Celio
// Date  : 2011 May 15
//
// Create an array of size <input>, where each array element is a pointer to
// another element in the array (circular linked list). Arrange pointers
// randomly to minimize cache-prefetching effects, and walk the array for set
// period of elements to normalize for work/array-size. 
//
// Allow user to set the number of independent fetch streams to show when
// the maximum number of requests is being met. Interleave streams to see how
// they interfere with one another.


//#define DEBUG 
//#define PRINT_ARRAY
#define PRINT_SCRIPT_FRIENDLY
//#define VERIFY_ARRAYS

// stride in # of elements, most processors have a 64byte cache line, so 16 elements
// would stride across cache lines.
//#define CACHELINE_SZ_IN_BYTES (64)
// larger "cacheline" sz to defeat spatial prefetchers
#define CACHELINE_SZ_IN_BYTES (256)
#define CACHELINE_SZ (CACHELINE_SZ_IN_BYTES/4)

#define WORKINGSET_SZ (64*1024*1024)

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
 
// Global Variables
uint32_t  g_num_requests;
uint32_t  g_num_elements_per_page;  
uint32_t  g_num_elements_working_set;  
uint32_t  g_interleaving_sz; //measured in # of 32b words
uint32_t  g_num_iterations;


double volatile run_time_ns;
double volatile run_time_us;
double volatile run_time_ms;
double volatile run_time_s;


// Function Declarations
uint32_t initializeGlobalArrays(uint32_t* arr_n_ptr, uint32_t num_elements, uint32_t num_elements_per_page, uint32_t stride, uint32_t interleaving_sz, uint32_t arr_offset);
uint32_t threadMain(uint32_t* arr_n_ptr, const uint32_t num_requests);
uint32_t printArray(uint32_t iter, uint32_t *arr_ptr, uint32_t num_elements, uint32_t stride, uint32_t offset);
uint32_t verifyArray(uint32_t *arr_ptr, uint32_t num_elements, uint32_t num_elements_per_page, uint32_t stride, uint32_t interleaving_sz, uint32_t offset);


int main(int argc, char* argv[])
{
#ifdef DEBUG  
   printf("\nBegin Test\n");
#endif

   if (argc != 4) 
   {
      fprintf(stderr, "argc=%d\n", argc);
//    fprintf(stderr, "\n[Usage]: mem_interleaving <Number of Mem Requests Issued in Parallel> <Number of Array Elements (Per Page)> <Interleaving Size (in Elements)> <Number of Iterations>\n\n");
    fprintf(stderr, "\n[Usage]: mem_interleaving <Number of Mem Requests Issued in Parallel> <Number of Array Elements (Per Page)> <Number of Iterations>\n\n");
      return 1;
   }

   g_num_requests          = atoi(argv[1]);
   g_num_elements_per_page = atoi(argv[2]);
//   g_interleaving_sz       = atoi(argv[3]);
   g_num_iterations        = atoi(argv[3]);

   uint32_t stride = CACHELINE_SZ;
   
   if (stride > g_num_elements_per_page)
      g_num_elements_per_page = stride;

   g_interleaving_sz = g_num_requests * g_num_elements_per_page;

#ifdef DEBUG
   fprintf(stderr, "Number of Parallel Requests = %d\n",   g_num_requests);
   fprintf(stderr, "Size of each page           = %d\n",   g_num_elements_per_page);
   fprintf(stderr, "Number of Iterations        = %d\n\n", g_num_iterations);
#endif

   g_num_elements_working_set  = WORKINGSET_SZ/sizeof(uint32_t);
//   printf("before   g_num_elements_working_set = %d\n", g_num_elements_working_set);
   uint32_t min_size           = g_num_requests * g_num_elements_per_page;
   g_num_elements_working_set  = g_num_elements_working_set + (min_size - (g_num_elements_working_set % min_size));

   if (g_num_elements_working_set % min_size != 0)
      printf("error in my math\n");
//   else
//      printf("math is good to go!\n");

//   printf("adjusted g_num_elements_working_set = %d\n", g_num_elements_working_set);

   uint32_t* arr_n_ptr = (uint32_t *) malloc((g_num_elements_working_set) * sizeof(uint32_t));

   
   
   // Provide each request its own "array" to pointer chase on This prevents
   // the processor from consolidating request streams The fact that we are
   // using a single array to hold all of this is a bit too "clever", but it
   // saves cycles in the critical loop from figuring out which array to use.
   for (int i=0; i < g_num_requests; i++)
   {
      uint32_t num_elements_per_req = g_num_elements_working_set / g_num_requests;

      uint32_t arr_offset = i*g_num_elements_per_page;

      //interleave pages 
      initializeGlobalArrays( arr_n_ptr, 
                              num_elements_per_req,
                              g_num_elements_per_page,
                              stride,
                              g_interleaving_sz,
                              arr_offset);
   }
   

   // this volatile ret_val is crucial, otherwise the entire run-loop 
   // gets optimized away! :(
   uint32_t volatile ret_val = threadMain(arr_n_ptr, g_num_requests);  

#ifdef PRINT_SCRIPT_FRIENDLY
   fprintf(stdout, "App:[mem_interleaving],NumRequests:[%d],AppSize:[%d],Time:[%g], TimeUnits:[Time Per Request (ns)], Bandwidth:[%g], BandwidthUnits:[Bandwidth (Req/s)],NumIterations:[%u]\n",
      g_num_requests,
      g_num_elements_per_page,
      ((double) run_time_ns / (double) g_num_iterations / (double) g_num_requests),
      ((double) g_num_requests * (double) g_num_iterations / (double) run_time_s),
	   g_num_iterations
      );
#endif

#ifdef DEBUG
  fprintf(stderr, "Done. Exiting...\n\n");
#endif

  return 0;
}


uint32_t threadMain(uint32_t* arr_n_ptr, const uint32_t num_requests)
{
   // clk_freq irrelevant 
   uint32_t const clk_freq = 0;
   cctime_t volatile start_time;

   uint32_t *idx = malloc(num_requests * sizeof(uint32_t));
  
   for (int i=0; i < num_requests; i++)
   {
      idx[i] = i * g_num_elements_per_page;
   }

   start_time = cc_get_seconds(clk_freq);

   for (uint32_t k = 0; k < g_num_iterations; k++)
   {
      for (uint32_t i=0; i < num_requests; i++)
      {
         idx[i]  = arr_n_ptr[idx[i]];
      }
   }

   cctime_t volatile stop_time = cc_get_seconds(clk_freq);
        
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
   uint32_t sum = 0;
   for (int i=0; i < num_requests; i++)
      sum += idx[i];

   return sum; 
}

       
//generate a random linked list of a PAGE_SZ, starting at address arr_n_ptr[page_offset]
//the calling function is in charge of stitching the page together with the other pages
//num_accesses is the number of pointers we are going to add (not the number of
//elements spanned by the page)
//returns the index to the last index (to help the calling function stitch pages together)
uint32_t initializePage(uint32_t* arr_n_ptr, uint32_t page_offset, uint32_t num_accesses, uint32_t stride)
{
#ifdef DEBUG
   printf("\n--(offset = %d) Generating Page-- num_accesses=%d, pageSz=%d, strd=%d\n\n",
      page_offset, num_accesses, num_accesses*stride*sizeof(int32_t), stride);
#endif

   cc_lfsr_t lfsr;
   uint32_t lfsr_init_val = 1; //TODO provide different streams different starting positions?
   uint32_t lfsr_width = (log(num_accesses) / log(2)); 

   uint32_t max_accesses = (0x1 << lfsr_width);

   // special case to handle non-powers of 2 num_accesses
   if (max_accesses < num_accesses)
      lfsr_width++;


   uint32_t idx;
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
 
 
uint32_t initializeGlobalArrays(uint32_t* arr_n_ptr, uint32_t num_elements, uint32_t num_elements_per_page, uint32_t stride, uint32_t interleaving_sz, uint32_t arr_offset)
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
      

   uint32_t num_accesses_per_page = num_elements_per_page/stride;
   uint32_t num_pages = num_elements / num_elements_per_page;

     
   uint32_t last_idx;

   // two-level randomization... (this is the outer level for-loop)
   // for each page...
   for(int page_idx=0; page_idx < num_pages; page_idx++)
   {
      uint32_t page_offset = arr_offset + page_idx*interleaving_sz;


      last_idx = initializePage(arr_n_ptr, page_offset, num_accesses_per_page, stride);
      // tie this page to the next page...
      arr_n_ptr[last_idx] =  page_offset + interleaving_sz;
#ifdef DEBUG
      printf("  *array[%4d] = %4d, \n", last_idx, arr_n_ptr[last_idx]);
#endif
      }


      //handle the last page ...
      //wrap the array back to the start
      arr_n_ptr[last_idx] = 0 + arr_offset;
#ifdef DEBUG
      printf(" **array[%4d] = %4d (looping back to the start: arr_offset=%d \n", 
         last_idx, arr_n_ptr[last_idx], arr_offset);
#endif

#ifdef PRINT_ARRAY
   printArray(0, arr_n_ptr, num_elements, stride, arr_offset);
#endif
#ifdef VERIFY_ARRAYS
   verifyArray(arr_n_ptr, num_elements, num_elements_per_page, stride, interleaving_sz, arr_offset);
#endif

   return 0;
}
 




uint32_t printArray(uint32_t iter, uint32_t *arr_ptr, uint32_t num_elements, uint32_t stride, uint32_t offset)
{
   fprintf(stderr, "Chasing through Array (run thru for 2x*num_el) after iteration: %d, Starting at idx = offset = %d\n", iter, offset);
   uint32_t idx = 0 + offset;
   for (uint32_t i = 0; i < 2*num_elements; i++)
   {
      if (i==num_elements)
         fprintf(stderr, "\n\n");
      fprintf(stderr, "%3d, ", arr_ptr[idx]);
      idx = arr_ptr[idx];
   }
   fprintf(stderr, "\n");
   
   fprintf(stderr, "arr_n_ptr[offset]: ");
   for (uint32_t i = 0 ; i < g_num_elements_working_set; i++)
   {
      if (i % stride == 0) fprintf(stderr, "\n");
      fprintf(stderr, "%3d, ", arr_ptr[i]);
   }
   fprintf(stderr, "\n");
   
   return 0;
}


uint32_t verifyArray(uint32_t *arr_ptr, uint32_t num_elements, uint32_t num_elements_per_page, uint32_t stride, uint32_t interleaving_sz, uint32_t offset)
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
   verify_array = (uint32_t *) malloc((g_num_elements_working_set) * sizeof(uint32_t));
   
   while (!finished)
   {
      //printf("verifying: idx = %d\n", idx);
      //verify_array[idx - offset] = verify_array[idx - offset] + 1;
      verify_array[idx] = verify_array[idx] + 1;
      counter++;
      idx = arr_ptr[idx];

      if(counter >= num_elements || idx == first_idx)
         finished = 1;
   }

   uint32_t error = 0;
   uint32_t num_pages = num_elements / num_elements_per_page;

   for (uint32_t i = 0; i < num_pages; i++)
      for (uint32_t j = 0; j < num_elements_per_page; j+=stride)
      {
         uint32_t idx = j + i*interleaving_sz + offset;
         //printf("page_idx=%d, j=%d, v_arr[%d]\n", i, j, idx);
         if (verify_array[idx] != 1)
         {
            fprintf(stderr, "Error at Element [%d], accessed %d times\n", 
               idx,
               verify_array[idx]
               );
            error = 1;
         }
      }

   if (counter==(num_elements/CACHELINE_SZ) && !error) {
#ifdef DEBUG
      fprintf(stderr,"Array verified\n");
#else
      int x=0; //here for compiler reasons
#endif
   } 
   else 
   {
      fprintf(stderr,"Error: Array size:%d, Stride size:%d, Loops in:%d\n\n",
         num_elements,
         CACHELINE_SZ,
         counter);
   }

   return error | (counter==(num_elements/CACHELINE_SZ));
}

