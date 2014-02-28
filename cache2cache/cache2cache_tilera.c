
//CS267 Final Project
//Spring 2011
//measuring cache to cache latencey

// Tilera junk that hopefully tells malloc to use L2s as a     
// distributed L3 (makes a big difference!)                    
#ifdef USE_TILERA
#include <malloc.h>                                            
MALLOC_USE_HASH(1);                                            
 
#include <memory.h>
#include <ilib.h>
#include <sys/archlib.h>
#include <sys/profiler.h>
#include <tmc/cmem.h> 
#endif

#define _GNU_SOURCE
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>
#include <sched.h>
//#include <utmpx.h>
#include <features.h>



#ifdef USE_TILERA

  /*"Channel Tag FROM left TO right" */
  #define CHAN_TAG_SEND 1337
  #define CHAN_TAG_REC 1339

  ILIB_RAW_SEND_PORT(port_send, 0);
  ILIB_RAW_RECEIVE_PORT(port_receive, 0);

  /*
   * FUNCTION PROTOTYPES
   */
   
  /* connect channels and open sender port */
  int setUpSendChannel(int group, int sender_rank);
  /* open receiver port */
  int setUpReceiveChannel();

  /* send info to the neighbors, return 0 when finished.*/
  int send_info(int sender_rank, int *left_info_ptr, int *right_info_ptr);
  
  /* receive info from the neighbor, return the value */
  int receive_info(int receiver_rank);



/**************************
* Process Parameters
**************************/
//iLibGeometry params
#define TILE0_X 0
#define TILE0_Y 0
//#define TILE1_X 7
//#define TILE1_Y 6
#define TILE1_X 0
#define TILE1_Y 1
#define TILES_WIDTH 1
#define TILES_HEIGHT 1


// measure direct messaging costs
// as opposed to cache coherence costs
//#define DIRECT_MESSAGING

#endif //end of Tilera 

#define NUM_THREADS 2
//#define THREAD1_MASK 0
//#define THREAD2_MASK 1
#define CACHELINE_SZ (64/4)
#define LATENCY
//#define BANDWIDTH
#define STRIDE 16

uint32_t num_elements; 
uint32_t num_iters;

volatile uint32_t flag1[CACHELINE_SZ];
volatile uint32_t flag2[CACHELINE_SZ];

uint32_t *array;


#ifdef USE_TILERA 

typedef struct
{
  int num_elements;
  int num_iters;
}
initStruct;


int setUpSendChannel(int group, int sender_rank)
{
  /* Define a channel for sending data and connect to the port */
   
  int receiver_rank = 0; 
  if(sender_rank == 0) 
    receiver_rank = 1;
    
    if (ilib_rawchan_connect(group,
                             sender_rank, CHAN_TAG_SEND,
                             receiver_rank, CHAN_TAG_REC) < 0)
    {
      ilib_die("Failed to define channel.");
    }
    
    // Open our send port.
    
    if ( ilib_rawchan_open_sender(CHAN_TAG_SEND, port_send) < 0)
      ilib_die("Failed to open SendPort.");
  
  return 0;
}

int setUpReceiveChannel()
{
    /* Connect to port to receive data */
      if (ilib_rawchan_open_receiver(CHAN_TAG_REC, port_receive) < 0)
        ilib_die("Failed to open ReceivePort.");
  
  return 0;
}


void spawn_procs(initStruct* init_blk) 
{
  if (ilib_group_size(ILIB_GROUP_SIBLINGS) == 1)
  {
    // A structure to be filled with process creation parameters
    // We will do *two* different spawns, one for each core so we have
    // full control over the location of each thread.
    ilibProcParam spawn_params[2];
  
    // We want to create more processes; leaving the binary_name field
    // NULL will cause iLib to exec using the currently running binary.
    spawn_params[0].num_procs = 1;
    spawn_params[1].num_procs = 1;
    spawn_params[0].binary_name = NULL;
    spawn_params[1].binary_name = NULL;
    spawn_params[0].argv = NULL;
    spawn_params[1].argv = NULL;
        
    spawn_params[0].tiles.x = TILE0_X;
    spawn_params[0].tiles.y = TILE0_Y;
    spawn_params[1].tiles.x = TILE1_X;
    spawn_params[1].tiles.y = TILE1_Y;
    spawn_params[0].tiles.width = 1;
    spawn_params[1].tiles.width = 1;
    spawn_params[0].tiles.height = 1;
    spawn_params[1].tiles.height = 1;
        

    spawn_params[0].init_block = init_blk;
    spawn_params[0].init_size = sizeof(*init_blk);
    spawn_params[1].init_block = init_blk;
    spawn_params[1].init_size = sizeof(*init_blk);

    ilib_proc_exec(2, &spawn_params); //proc_exec(int #of params, params[])
        
    // If we get here, exec failed (it shouldn't return - exec replaces the
    //  current process with new ones)
    ilib_die("ilib_proc_exec() failed!");
  }
}

#endif





void pingPongX(int tid)
{
#ifndef DIRECT_MESSAGING  
  int waiting_thread = 1;
#endif
  int count = 0;


  //set flags and initialize array 
  if(tid == 0){
    int i=0;
    for(i=0; i<num_elements-1; i++)
    {
#ifdef LATENCY
      array[i] = i+STRIDE;
#else
      array[i] = 0;
#endif
    }
    array[num_elements -1] = 0;
  }

#ifdef DIRECT_MESSAGING
  int* localarray = (int*) malloc(sizeof(int)*num_elements);

  if (tid == 0)
  {
    int i;
    for (i = 0; i < num_elements; i++)
      localarray[i] = array[i];
  }
#endif
                               
  ilib_msg_barrier(ILIB_GROUP_SIBLINGS);
  

  cctime_t start_time;
  
  if (tid ==0) {
    start = ccget_time();
  }
  

#ifdef DIRECT_MESSAGING

  int sender_id = 0;
  int buf_size = sizeof(int)*num_elements;
  
  while(count < num_iters)
  {
    count++;
     
     if (tid == sender_id)
     {
       ilib_rawchan_send_buffer(port_send, localarray, buf_size); 
     }
     else
     {
       ilib_rawchan_receive_buffer(port_send, localarray, buf_size); 

       for (int i=0; i < num_elements; i++)
         localarray[i] = localarray[i] + 1;
     }

    if (sender_id == 0)
      sender_id = 1;
    else
      sender_id = 0;
     
  }

#else
  
  ilibRequest request;
  ilibStatus  status;
               
  while(count < num_iters){

    count++;
    
    //threads are forced to alternate so write permissions must be passed back and forth
    //local spin wait while it's not your turn
    
    if(tid == waiting_thread) 
    {
      ilib_msg_start_barrier(ILIB_GROUP_SIBLINGS, &request);
      while(ilib_test(&request,&status)) ;
    }

    int idx = 0;
    int i=0;

    do{
      //get writing permissions
      #ifdef BANDWIDTH
      array[i] = count;
      //array[i+1] = count;
      i+=STRIDE;
      #endif
      
      //force serialization
      #ifdef LATENCY
      idx = array[idx];
      i+=STRIDE;
      array[idx] =  i; 
      //idx = array[idx];
      //array[idx] = i+STRIDE;
      //i+=STRIDE;
      #endif
      
    }while(i<num_elements-STRIDE);
    

    waiting_thread = tid;
    ilib_msg_start_barrier(ILIB_GROUP_SIBLINGS, &request);
    while(ilib_test(&request,&status)) ;

  }

//  printf("DONE!!! Rank; %d Count: %d, Count mod 2 == %d\n", tid, count, (count % 2));
//  if ((count % 2 == 1 && tid == 1) || (count % 2 == 0 && tid == 0))
  if (tid == 0)
  {
    ilib_msg_start_barrier(ILIB_GROUP_SIBLINGS, &request);
    while(ilib_test(&request,&status)) ;
  }

#endif //end cache coherence test

  
  if(tid == 0)
  {
      double stop = mysecond();
//    long stop = get_cycle_count();
    int num_lookedat = num_elements/STRIDE;
    if(num_lookedat <1)
      num_lookedat = 1;
//    double runtime = (stop-start)*(1/700e6);
    double runtime = (stop-start);
    double bandwidth = ((double)(num_elements*sizeof(int)*num_iters)/1048576/runtime);
    double latency = ((double) runtime *1e9 / ((double) num_lookedat * num_iters));

    #ifdef BANDWIDTH
    printf("Cache-to-cache bandwidth = %g\n", bandwidth);
    #endif

    #ifdef LATENCY
    printf("Cache-to-cache latency = %g\n", latency);
    #endif 
   

    #ifdef BANDWIDTH
    fprintf(stdout, "App:[cache2cache, ,NumThreads:[%d],AppSizeArg:[%d],Time(s):[%g],NumIterations:[%u],Units:[MB/s]\n",
	      num_cores,
	      num_elements,
	      bandwidth,
	      num_iters
	      );
    #endif

    #ifdef LATENCY
    fprintf(stdout, "App:[cache2cache, ,NumThreads:[%d],AppSizeArg:[%d],Time(s):[%g],NumIterations:[%u],Units:[ns/access]\n",
        num_cores,
        num_elements,
        latency,
        num_iters
        );
    #endif

    
  }
}



 
int tilera_main(int argc, char* argv[]){

  initStruct before_init_blk;

  if (ilib_group_size(ILIB_GROUP_SIBLINGS) == 1)
  {
    if(argc != 3)
    {
      fprintf(stderr, "argc=%d\n", argc);
      fprintf(stderr, "\n[Usage]: cache2cache <Number of Array Elements (Total)> <Nu\
  mber of Iterations>\n\n");
      return 1;
    }

    num_elements = atoi(argv[2]);
    num_iters = atoi(argv[3]);
     
    printf("Greetings from Starting Proc, num_el: %d, num_iters: %d\n", 
      num_elements,
      num_iters);
 
    before_init_blk.num_elements  = num_elements;
    before_init_blk.num_iters     = num_iters;
    
//    tmc_cmem_init(0);
//    array = (int*) tmc_cmem_malloc(sizeof(int)*num_elements);
  }


  /**************************
   * SET UP PARRALLEL PROCCESSING
   * go parrallel and set up channels
   **************************/

  spawn_procs(&before_init_blk);   
  int my_rank = ilib_group_rank(ILIB_GROUP_SIBLINGS);

  if (ilib_group_size(ILIB_GROUP_SIBLINGS) != 1)
  {
    size_t init_size = 0;
    initStruct* after = ilib_proc_get_init_block(&init_size);
    num_elements  = after->num_elements;
    num_iters     = after->num_iters;
  }
    
  
  /* Set up the channels for the direct messaging tests.
   * Stagger connecting send and receive ports, since I am using
   * BLOCKING calls to cut down on bugs.  
   */
   
  if(my_rank % 2 == 0) {
    setUpSendChannel(ILIB_GROUP_SIBLINGS, my_rank); //connect and open sender channel.
    setUpReceiveChannel();//receiver then connects as rec to channels
  } else {  
    setUpReceiveChannel();//receiver then connects as rec to channels
    setUpSendChannel(ILIB_GROUP_SIBLINGS, my_rank); //connect and open sender channel.
  }
    
  // allocate the shared memory (for the cache coherence test)
  if (my_rank == 0)
  {
    tmc_cmem_init(0);
    array = (int*) tmc_cmem_malloc(sizeof(int)*num_elements);
  } 

  // Broadcast the shared memory address from tile 0 to the other
  // tiles.  We ignore the result and status object for brevity.
  ilibStatus status;
  ilib_msg_broadcast(ILIB_GROUP_SIBLINGS, 0,
    &array, sizeof(array), &status);
 
  pingPongX(my_rank);

  ilib_finish();

  exit(0);  
}


int main(int argc, char* argv[]){

#ifdef USE_TILERA
   ilib_init();
   tilera_main();
#endif


   if(argc != 4)
   {
      fprintf(stderr, "argc=%d\n", argc);
      fprintf(stderr, "\n[Usage]: cache2cache <Number of Array Elements (Total)> <Number of Iterations>\n\n");
      return 1;
   }

    num_elements = atoi(argv[2]);
    num_iters = atoi(argv[3]);
     
    printf("Greetings from Starting Proc,   num_el: %d, num_iters: %d\n", 
      num_elements,
      num_iters);
  


  /**************************
   * SET UP PARRALLEL PROCCESSING
   **************************/

    array = (int*) tmc_cmem_malloc(sizeof(int)*num_elements);

 
  pingPongX(my_rank);

  ilib_finish();

  exit(0);  
}
