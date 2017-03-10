#include <stdint.h>
#include <string.h>

#include "fdy_pool.h"

/***Author: cloudleware , so far this is only applicable per thread, not for multithread ***/


void* mem_align(size_t size);

fdy_pool* create_pool( size_t size ) {
    fdy_pool * p = (fdy_pool*)mem_align(size + sizeof(fdy_pool));
    p->next = (void*)&p[1]; //p + sizeof(fdy_pool)
    p->end = (void*)((uintptr_t)p->next + size);
    return p;    // p->memblk = //mem_align( 16, DEFAULT_BLK_SZ);
}

void destroy_pool( fdy_pool *p ) {
    free(p);
}

size_t mem_left( fdy_pool *p ) {
    return (uintptr_t) p->end - (uintptr_t) p->next;
}

size_t blk_size( fdy_pool *p ) {
    return (uintptr_t) p->end - (uintptr_t) (void*)&p[1];
}

fdy_pool* re_create_pool( fdy_pool *curr_p) {
    size_t totalSize = blk_size(curr_p);
    size_t newSize = totalSize + totalSize;
    fdy_pool *newp = (fdy_pool*)mem_align(sizeof(fdy_pool) + newSize);
    memcpy((void*)newp, (void*)curr_p, sizeof(fdy_pool));
    memcpy((void*)&newp[1], (void*)&curr_p[1], totalSize);
    newp->next = (void*)&newp[1]; //p + sizeof(fdy_pool)
    newp->end =  (void*)((uintptr_t)newp->next + newSize);
    newp->next = (void*) ((uintptr_t) newp->next + totalSize);
    destroy_pool(curr_p);
    return newp;
}

void * falloc( fdy_pool **p, size_t size ) {
    fdy_pool *curr_p = *p;
    if ( mem_left(*p) < size ) {
        *p = curr_p = (fdy_pool*)re_create_pool(*p);
    }
    void *mem = (void*)curr_p->next;
    curr_p->next = (void*) ((uintptr_t)curr_p->next +  size);
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
        printf("posix_memalign(%uz, %uz) failed \n", ALIGNMENT, size);
        p = NULL;
    }
    printf("posix_memalign: %p:%uz @%uz \n", p, size, ALIGNMENT);
#else
    // printf("%s\n", "Using Malloc");
    p = malloc(size + sizeof(fdy_pool));

#endif
    return p;
}



// int main()
// {
//     fdy_pool *thisp = create_pool(DEFAULT_BLK_SZ);

//     printf("thisp Address = %u\n",  thisp);
//     printf("thisp->next Address = %u\n",  thisp->next);
//     char* s = (char*)falloc(&thisp, DEFAULT_BLK_SZ);
//     char* s2 = (char*)falloc(&thisp, DEFAULT_BLK_SZ);
//     char* s3 = (char*)falloc(&thisp, DEFAULT_BLK_SZ);
//     fdy_pool *thatp = create_pool(DEFAULT_BLK_SZ);
//     char* s4 = (char*)falloc(&thisp, DEFAULT_BLK_SZ);
//     char* s5 = (char*)falloc(&thisp, DEFAULT_BLK_SZ);

//     char* s6 = (char*)falloc(&thatp, DEFAULT_BLK_SZ);
//     char* s7 = (char*)falloc(&thatp, DEFAULT_BLK_SZ);

//     printf("thisp Address = %u\n",  thisp);
//     printf("thisp->next Address = %u\n",  thisp->next);

//     printf("thatp Address = %u\n",  thatp);
//     printf("thatp->next Address = %u\n",  thatp->next);



//     destroy_pool(thatp);
//     destroy_pool(thisp);

//     return (0);
// }