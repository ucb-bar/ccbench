//Spring 2011
//measuring cache to cache latencey and bandwidth

// cache2cache is used for both measuring bandwidth and for measuring latency.
// Currently, it only measures one at a time, which requires using the
// pre-processor. STRIDE is later changed to "1" if using the BANDWIDTH test.


// NOTE: only use local_array, to save on a load from global

//#define LATENCY
#define BANDWIDTH
#define BAND_STRIDE 1
#define LAT_STRIDE 16

// run benchmark with local communication ("control" experiment, if you will)
//#define LOCAL

#define _GNU_SOURCE
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <cctimer.h>

#if !defined(__APPLE__)
#include <sched.h>
#include <utmpx.h>
#include <features.h>
#endif


#define NUM_THREADS 2
#define THREAD1_MASK 0
#define THREAD2_MASK 2
#define LINESIZE (64/4)

#define CACHELINE_SZ (64)
#define CACHELINE_SZ_IN_EL (CACHELINE_SZ/(sizeof(int)))



int g_num_elements; 
int num_iters;
#if !defined(__APPLE__)
cpu_set_t cpu1, cpu2;
#endif
pthread_attr_t attr, attr2;

volatile int flag1[LINESIZE];
volatile int flag2[LINESIZE];

pthread_t thread1, thread2, thread3, thread4;
int *array;


// this piece of code, only for one thread, should match the numbers reported by STREAM
void *stream_test(void* t_int)
{
   int tid = (int) *(int*) t_int;
   if (tid > 1) {
      //kick other threads out
      return 0;
   }

   int constvalue = rand();
   uint32_t const clk_freq = 0;
   int const num_elements = g_num_elements;
   cctime_t volatile start_time = cc_get_seconds(clk_freq);

   int count = 0;

   // movq  (%rsi),%rax         LD  rax, array
   // movl  %r12d,(%rax,%rdx)   ST  (rax+rdx), random_numer
   // addl  $0x01,%ecx          ADD i, 1
   // addq  $0x04,%rdx          ADD rdx, 1 
   // cmpl  %ebx,%ecx           BNE num_elements, ecx
   // jne   0x100001090         

   int const iters = num_iters;
   int* local_array = array;
   for (int k=0; k < iters; k++)
   {
      constvalue = rand(); //means do more work everytime
      for (int i=0; i < num_elements; i++)
      {
//         array[i] = constvalue;
         local_array[i] = constvalue;
      }
   }
  
  
   cctime_t volatile stop_time = cc_get_seconds(clk_freq);
   double runtime = (stop_time-start_time);
   
   double accesses = 1.0;
   double size_per_fetch = sizeof(int)*accesses;
   double bandwidth 
      = ((double)((double)num_elements*size_per_fetch*num_iters)/1048576.0/runtime);
  
   printf("Testing Single Thread Write bandwidth\n");
   printf("Array Size      : %g kB\n", num_elements*sizeof(int)/1024.0);
   printf("Bandwidth       : %g MB/s\n", bandwidth);
   printf("size_per_fetch  : %g bytes\n", size_per_fetch);

   printf("Bytes Transfered: %g\n", (num_elements*size_per_fetch*num_iters));
   printf("runtime (sec)   : %g\n", runtime);


   fprintf(stdout, "App:[cache2cache_streamtest, AppSize:[%d],ArraySz:[%d] (kB), Output:[%g],NumIterations:[%u],Units:[MB/s]\n",
	    num_elements,
       num_elements*sizeof(int),
	    bandwidth,
	    num_iters
	    );

}

void *pingPongX(void* t_int)
{
  
   //local pointer to locks 
   volatile int *mylock;
   volatile int *otherLock;
   int tid = (int) *(int*) t_int;
   int waiting_thread = 2;
   int count = 0;


   const int num_elements = g_num_elements;
   int *local_array = array;

// stride in elements
#ifdef BANDWIDTH
  const uint32_t stride_in_el = BAND_STRIDE;
#else
  const uint32_t stride_in_el = LAT_STRIDE;
#endif

  //set flags and initialize array 
  if(tid == 1)
  {
    //    printf("thread 1 is on %d\n", sched_getcpu());
    mylock = flag1; 
    otherLock = flag2;
    int i=0;
    for(i==0; i<num_elements-1; i++)
      array[i] = i+stride_in_el;
    array[num_elements -1] = 0;
  }
  else
  {
    // printf("thread 2 is on %d\n", sched_getcpu());
    mylock = flag2; 
    otherLock = flag1;
  }

  // clk_freq irrelevant 
  uint32_t const clk_freq = 0;
  cctime_t volatile start_time = cc_get_seconds(clk_freq);
  
   while(count < num_iters)
   {
    
      //threads are forced to alternate so write permissions must be passed back and forth
      //local spin wait while it's not your turn
    
      if(tid == waiting_thread)
         while(*mylock == 0);

      *otherLock = 0;
      //    printf("%ld\t x=%d\t mylock = %d\t otherlock = %d\n", pthread_self(), count, *mylock, *otherLock);
      count++;
      int idx = 0;
      int i=0;

      __sync_synchronize();
      
      for (int i=0; i < num_elements; i++)
      {
         //get write permissions
         #ifdef BANDWIDTH
         local_array[i] = count;
         #endif
         
         //force serialization
         #ifdef LATENCY
         i should not compile... fix i, since i added for loop
         idx = array[idx];
         i+=stride_in_el;
         array[idx] =  i; 
         #endif
      }
      
      __sync_synchronize();
      
//    }while(i<num_elements-stride_in_el);
    

    waiting_thread = tid;
    *mylock = 0;
    *otherLock = 1;

  }
  cctime_t volatile stop_time = cc_get_seconds(clk_freq);
  if(tid == 1)
  {
    int num_lookedat = num_elements/stride_in_el;
    if(num_lookedat <1)
      num_lookedat = 1;
    double runtime = (stop_time-start_time);
    
        
    double size_per_fetch = sizeof(int)*2;
    //TODO this logic isn't correct for sub cacheline strides...
    if (stride_in_el >= (CACHELINE_SZ)/sizeof(int))
       size_per_fetch = CACHELINE_SZ;
    else  {
       printf("size_per_fetch is set to sizeof(int) = %d\n", sizeof(int));
       size_per_fetch = sizeof(int)*2;
    }
 
    
    
    double bandwidth 
         = ((double)((double) num_elements/stride_in_el*size_per_fetch*num_iters)/1048576/runtime);
    double latency = ((double) runtime *1e9 / ((double) num_lookedat * num_iters));
 


    #ifdef BANDWIDTH
    printf("Cache-to-cache bandwidth = %g MB/s\n", bandwidth);
    printf("Num_element     : %d\n", num_elements);
    printf("stride          : %d\n", stride_in_el);
    printf("Num_looked at   : %d\n", num_lookedat);
    printf("size_per_fetch  : %g\n", size_per_fetch);
    printf("Bytes Transfered: %g\n", (num_elements/stride_in_el*size_per_fetch*num_iters));
    printf("runtime (sec)   : %g\n", runtime);
    #endif

    #ifdef LATENCY
    printf("Cache-to-cache latency = %g\n", latency);
    #endif 


    #ifdef BANDWIDTH
    fprintf(stdout, "App:[cache2cache, AppSize:[%d],ArraySz:[%d] (kB), Output:[%g],NumIterations:[%u],Units:[MB/s]\n",
	    num_elements,
       num_elements*sizeof(int),
	    bandwidth,
	    num_iters
	    );
    #endif

    #ifdef LATENCY
    fprintf(stdout, "App:[cache2cache, AppSize:[%d],Output:[%g],NumIterations:[%u],Units:[ns/access]\n",
      num_elements,
      latency,
      num_iters
      );
    #endif

    
  }
}

//use a local counter to determine the runtime of doing this with a variable in your own cache
void *localCounter(void* t_int)
{

  int tid = (int) *(int*) t_int;
  int *local_array=(int*)malloc(g_num_elements*sizeof(int));
  
  volatile int* mylock;
  volatile int* otherlock;

  int waiting_thread = 2;
  int count = 0;

// stride in elements
#ifdef BANDWIDTH
  const uint32_t stride_in_el = BAND_STRIDE;
#else
  const uint32_t stride_in_el = LAT_STRIDE;
#endif


  //set flags and initialize array
  if(tid == 1)
  {
    //    printf("thread 1 is on %d\n", sched_getcpu());                                                                                                                                                      
    mylock = flag1;
    otherlock = flag2;
    int i=0;
    for(i==0; i<g_num_elements-1; i++)
      local_array[i] = i+stride_in_el;
    local_array[g_num_elements -1] = 0;
  }
  else
  {
    mylock = flag2;
    otherlock = flag1;
  }

  
  // clk_freq irrelevant 
  uint32_t const clk_freq = 0;
  cctime_t volatile start_time = cc_get_seconds(clk_freq);

  while(count<num_iters)
  {
    if(waiting_thread == tid)
      while(mylock == 0);
    count++;
    int i=0;
    int idx = 0;
    
    do
    {
      #ifdef BANDWIDTH
      local_array[i] = count;
      i+=stride_in_el;
      #endif

      #ifdef LATENCY
      idx = local_array[idx];
      i+=stride_in_el;
      local_array[idx] =  i;
      #endif
    }while(i<g_num_elements-stride_in_el);
    
    waiting_thread = tid;
    *mylock = 0;
    *otherlock = 1;
    //    printf("%d\t%d\n", *flag1, *flag2);
  }

  cctime_t volatile stop_time = cc_get_seconds(clk_freq);
  
  if(tid == 1)
  {
    int num_lookedat = g_num_elements/stride_in_el;
    if(num_lookedat <1)
      num_lookedat = 1;
    double runtime = (stop_time-start_time);
    
    double size_per_fetch;
    //TODO this logic isn't correct for sub cacheline strides...
    if (stride_in_el >= (CACHELINE_SZ)/sizeof(int))
       size_per_fetch = CACHELINE_SZ;
    else 
       size_per_fetch = sizeof(int);
   

    double bandwidth = ((double)(g_num_elements/stride_in_el*size_per_fetch*num_iters)/1048576/runtime);
    double latency = ((double) runtime *1e9 / ((double) num_lookedat * num_iters));


    #ifdef BANDWIDTH
    printf("Cache-to-cache bandwidth = %g\n", bandwidth);
    printf("Num_element  : %d\n", g_num_elements);
    printf("stride       : %d\n", stride_in_el);
    printf("Num_looked at: %d\n", num_lookedat);
    #endif

    #ifdef LATENCY
    printf("Cache-to-cache latency = %g\n", latency);
    #endif


    #ifdef BANDWIDTH
    fprintf(stdout, "App:[cache2cache, AppSize:[%d],Output:[%g],NumIterations:[%u],Units:[MB/s]\n",
      g_num_elements,
      bandwidth,
      num_iters
      );
      #endif

      #ifdef LATENCY
    fprintf(stdout, "App:[cache2cache, AppSize:[%d],Output:[%g],NumIterations:[%u],Units:[ns/access]\n",
      g_num_elements,
      latency,
      num_iters
      );
      #endif

  }

}



int main(int argc, char* argv[])
{

  if(argc != 3)
  {
    fprintf(stderr, "argc=%d\n", argc);
    fprintf(stderr, "\n[Usage]: cache2cache <Number of Array Elements (Total)> <Nu\
mber of Iterations>\n\n");
    return 1;
  }


  g_num_elements = atoi(argv[1]);
  num_iters = atoi(argv[2]);

  //make sure large arrays don't run for ever, in future, balance this to run for 1secondA
  if (g_num_elements < (5100-1))
     num_iters = num_iters;
  else if (g_num_elements < (75100-1))
     num_iters = num_iters/2;
   else if (g_num_elements < 262144)
     num_iters = 100;
   else 
     num_iters = 50;

  array = (int*)malloc(sizeof(int)*g_num_elements);

  int a_err, b_err;
  
  int tid[2] = {1,2};
  int j;
  flag1[0] = 1;
  flag2[0] = 0;

#if !defined(__APPLE__)
  CPU_ZERO(&cpu1);
  CPU_SET(THREAD1_MASK, &cpu1);
  CPU_ZERO(&cpu2);
  CPU_SET(THREAD2_MASK, &cpu2);

  pthread_attr_init(&attr);
  pthread_attr_init(&attr2);
  pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpu1);
  pthread_attr_setaffinity_np(&attr2, sizeof(cpu_set_t), &cpu2);
#else
  pthread_attr_init(&attr);
  pthread_attr_init(&attr2);
#endif

  #ifndef LOCAL
  a_err = pthread_create(&thread1, &attr, pingPongX, (void*)&tid[0]);
  b_err = pthread_create(&thread2, &attr2, pingPongX,  (void*)&tid[1]);
//  a_err = pthread_create(&thread1, &attr, stream_test, (void*)&tid[0]);
//  b_err = pthread_create(&thread2, &attr2, stream_test,  (void*)&tid[1]);
  
  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);
  #endif
  
  #ifdef LOCAL
  //Local Version -- no thread communication
    a_err = pthread_create(&thread3, NULL, localCounter, (void*)&tid[0]);
  b_err = pthread_create(&thread4, NULL, localCounter, (void*)&tid[1]);
  
  pthread_join(thread3, NULL);
  pthread_join(thread4, NULL);
  #endif

  exit(0);  
}
