////////////////////////////////////////////////////////////////////////////////
// Thread Interface
// Author: Christopher Celio
// Date  : 2011 May 13

// Abstract out the thread code. Ideally this should greatly simplify the
// micro-benchmark code while allowing easy porting to platforms that either do
// not support pthreads (barebones simulators) or support different constructs
// (e.g., the Tilera iLib library).
//
// Worse, stuff like pinning threads to cores is verbose and can be painful to
// make portable. So let's hide that stuff in here.

// NOTE: currently written to only handle pthreads

#include <stdint.h>
#include <pthread.h>

typedef pthread_t ccthread_t;

//int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
//                   void *(*start_routine)(void*), void *arg);
uint32_t cc_thread_create();
uint32_t cc_thread_join();


