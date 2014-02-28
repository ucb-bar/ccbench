////////////////////////////////////////////////////////////////////////////////
// Barrier Implementation
// Author: Christopher Celio
// Date  : 2010 Nov 4

// abstracted for portability, especially since I can't trust pthread_barrier to 
// be available on all platforms (I'm looking at you Mac OSX).

// This initial implementation is courtesy of:
// http://www.howforge.com/implementing-barrier-in-pthreads

#ifndef __riscv
#define _XOPEN_SOURCE 600

#include <pthread.h>

#if defined(WITH_BARRIER) || defined(CYGWIN) || defined(__APPLE__)
#define pthread_barrier_t barrier_t
#define pthread_barrier_attr_t barrier_attr_t
#define pthread_barrier_init(b,a,n) barrier_init(b,n)
#define pthread_barrier_destroy(b) barrier_destroy(b)
#define pthread_barrier_wait(b) barrier_wait(b)
#define pthread_barrier_attr_init(b)  
#endif


typedef struct {
    int needed;
    int called;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} barrier_t;
 
//unused, dummy attribute variable
typedef int barrier_attr_t;
                                  
int barrier_init   (barrier_t *barrier, int needed);
int barrier_destroy(barrier_t *barrier);
int barrier_wait   (barrier_t *barrier);

#endif // if !(__riscv)

