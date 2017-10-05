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
typedef void* (*CH_HASH_MALLOC_FN)(size_t);
typedef void (*CH_HASH_FREE_FN)(void*);
void ffunc_hash_set_alloc_fn(CH_HASH_MALLOC_FN malloc_fun, CH_HASH_FREE_FN free_fun);

// fhash is unsigned long int key based
typedef struct {
	unsigned char** bucket;
	int used;
	unsigned long total;
	size_t type_sz;
#ifdef SYNC
	pthread_mutex_t mutex;
#endif
} ffunc_hash; // Map array

typedef unsigned long int Mkey;

ffunc_hash* ffunc_hash_init(size_t length,  size_t object_size);
unsigned char* ffunc_hash_assign(ffunc_hash *fhash, Mkey key);
unsigned char* ffunc_hash_get(ffunc_hash *fhash, Mkey key);
// int ffunc_hash_read(ffunc_hash *fhash, Mkey key, unsigned char* __return);
// int ffunc_hash_splice(ffunc_hash *fhash, Mkey key, unsigned char* __return);
int ffunc_hash_remove(ffunc_hash *fhash, Mkey key);
void ffunc_hash_destroy(ffunc_hash *fhash);

#ifdef SYNC
static inline void ffunc_hash_lock(ffunc_hash *hash_) {
    pthread_mutex_lock(&(hash_->mutex));
}
static inline void ffunc_hash_unlock(ffunc_hash *hash_) {
    pthread_mutex_unlock(&(hash_->mutex));
}
#endif

#ifdef __cplusplus
}
#endif

#endif
