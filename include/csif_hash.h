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
typedef void* (*CSIF_HASH_MALLOC_FN)(size_t);
typedef void (*CSIF_HASH_FREE_FN)(void*);
void csif_hash_set_alloc_fn(CSIF_HASH_MALLOC_FN malloc_fun, CSIF_HASH_FREE_FN free_fun);

// fhash is unsigned long int key based
typedef struct {
	unsigned char** bucket;
	int used;
	unsigned long total;
	size_t type_sz;
#ifdef SYNC
	pthread_mutex_t mutex;
#endif
} csif_hash; // Map array

typedef unsigned long int Mkey;

csif_hash* csif_hash_init(size_t length,  size_t object_size);
unsigned char* csif_hash_assign(csif_hash *fhash, Mkey key);
unsigned char* csif_hash_get(csif_hash *fhash, Mkey key);
 int csif_hash_read(csif_hash *fhash, Mkey key, unsigned char* __return);
 int csif_hash_splice(csif_hash *fhash, Mkey key, unsigned char* __return);
int csif_hash_remove(csif_hash *fhash, Mkey key);
void csif_hash_destroy(csif_hash *fhash);

#ifdef SYNC
static inline void csif_hash_lock(csif_hash *hash_) {
    pthread_mutex_lock(&(hash_->mutex));
}
static inline void csif_hash_unlock(csif_hash *hash_) {
    pthread_mutex_unlock(&(hash_->mutex));
}
#endif

#ifdef __cplusplus
}
#endif

#endif
