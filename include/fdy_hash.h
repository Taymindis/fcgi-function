#ifndef _HEADER_FHASH_
#define _HEADER_FHASH_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#ifdef SYNC
#include <pthread.h>
#endif
typedef void* (*FHASH_MALLOC)(size_t);
typedef void (*FHASH_FREE)(void*);
void fhash_set_allocation_fn(FHASH_MALLOC malloc_fun, FHASH_FREE free_fun);

// fhash is unsigned long int key based
typedef struct {
	unsigned char** bucket;
	int used;
	int total;
	size_t type_sz;
#ifdef SYNC
	pthread_mutex_t mutex;
#endif
} fdy_hash; // Map array

typedef unsigned long int Mkey;

fdy_hash* init_fhash(size_t length,  size_t object_size);
void* newObject(fdy_hash *fhash, Mkey key);
void* getObject(fdy_hash *fhash, Mkey key);
int readObject(fdy_hash *fhash, Mkey key, void* __return);

int spliceObject(fdy_hash *fhash, Mkey key, void* __return);
int removeObject(fdy_hash *fhash, Mkey key);
void destroy(fdy_hash *fhash);

#ifdef SYNC
void fhash_lock(fdy_hash *fhash);
void fhash_unlock(fdy_hash *fhash);
#endif

#ifdef __cplusplus
}
#endif

#endif