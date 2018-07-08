#include <stdint.h>
#include <string.h>

#include "ffunc_pool.h"

/***Author: cloudleware , so far this is only applicable per thread, not for multithread ***/


void* mem_align(size_t size);

ffunc_pool* create_pool( size_t size ) {
    ffunc_pool * p = (ffunc_pool*)mem_align(size + sizeof(ffunc_pool));
    p->next = (void*)&p[1]; //p + sizeof(ffunc_pool)
    p->end = (void*)((uintptr_t)p->next + size);
    return p;    // p->memblk = //mem_align( 16, DEFAULT_BLK_SZ);
}

void destroy_pool( ffunc_pool *p ) {
    if (p)
        free(p);
}

size_t mem_left( ffunc_pool *p ) {
    return (uintptr_t) p->end - (uintptr_t) p->next;
}

size_t blk_size( ffunc_pool *p ) {
    return (uintptr_t) p->end - (uintptr_t) (void*)&p[1];
}

ffunc_pool* re_create_pool( ffunc_pool *curr_p) {
    ffunc_pool *newp = NULL;
    if (curr_p) {
        // ffunc_print("%s\n", "Recreate Pool");
        size_t curr_used_size = blk_size(curr_p);
        size_t newSize = curr_used_size + curr_used_size;
        newp = (ffunc_pool*)mem_align(sizeof(ffunc_pool) + newSize);
        memcpy((void*)newp, (void*)curr_p, sizeof(ffunc_pool));
        memcpy((void*)&newp[1], (void*)&curr_p[1], curr_used_size);
        newp->next = (void*)&newp[1]; //p + sizeof(ffunc_pool)
        newp->end =  (void*)((uintptr_t)newp->next + newSize);
        newp->next = (void*) ((uintptr_t) newp->next + curr_used_size);
        destroy_pool(curr_p);
    }
    return newp;

}

void * falloc( ffunc_pool **p, size_t size ){
    ffunc_pool *curr_p = *p;
    if ( mem_left(*p) < size ) {
        *p = curr_p = (ffunc_pool*)re_create_pool(*p);
    }
    void *mem = (void*)curr_p->next;
    curr_p->next = (void*) ((uintptr_t)curr_p->next +  size); // alloc new memory
    // memset(curr_p->next, 0, 1); // split for next

    return mem;
}

void*
mem_align(size_t size) //alignment => 16
{
    void  *p;
#ifdef POSIX_MEMALIGN
    int    err;
    err = posix_memalign(&p, ALIGNMENT, size);
    if (err) {
        ffunc_print("posix_memalign(%uz, %uz) failed \n", ALIGNMENT, size);
        p = NULL;
    }
    ffunc_print("posix_memalign: %p:%uz @%uz \n", p, size, ALIGNMENT);
#else
    // ffunc_print("%s\n", "Using Malloc");
    p = malloc(size + sizeof(ffunc_pool));

#endif
    return p;
}



//  int main()
//  {
//      ffunc_pool *thisp = create_pool(DEFAULT_BLK_SZ);

//      ffunc_print("thisp Address = %p\n",  thisp);
//      ffunc_print("thisp->next Address = %p\n",  thisp->next);
//      char* s = (char*)falloc(&thisp, DEFAULT_BLK_SZ);
//      char* s2 = (char*)falloc(&thisp, DEFAULT_BLK_SZ);
//      char* s3 = (char*)falloc(&thisp, DEFAULT_BLK_SZ);
//      ffunc_pool *thatp = create_pool(DEFAULT_BLK_SZ);
//      char* s4 = (char*)falloc(&thisp, DEFAULT_BLK_SZ);
//      char* s5 = (char*)falloc(&thisp, DEFAULT_BLK_SZ);

//      char* s6 = (char*)falloc(&thatp, DEFAULT_BLK_SZ);
//      char* s7 = (char*)falloc(&thatp, DEFAULT_BLK_SZ);

//      ffunc_print("thisp Address = %p\n",  thisp);
//      ffunc_print("thisp->next Address = %p\n",  thisp->next);

// //     ffunc_print("thatp Address = %u\n",  thatp);
// //     ffunc_print("thatp->next Address = %u\n",  thatp->next);


//      destroy_pool(thatp);
//      destroy_pool(thisp);

//      return (0);
//  }
