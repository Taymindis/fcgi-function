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
void ch_hash_set_alloc_fn(CH_HASH_MALLOC_FN malloc_fun, CH_HASH_FREE_FN free_fun);

// fhash is unsigned long int key based
typedef struct {
	unsigned char** bucket;
	int used;
	unsigned long total;
	size_t type_sz;
#ifdef SYNC
	pthread_mutex_t mutex;
#endif
} ch_hash; // Map array

typedef unsigned long int Mkey;

ch_hash* ch_hash_init(size_t length,  size_t object_size);
unsigned char* ch_hash_assign(ch_hash *fhash, Mkey key);
unsigned char* ch_hash_get(ch_hash *fhash, Mkey key);
// int ch_hash_read(ch_hash *fhash, Mkey key, unsigned char* __return);
// int ch_hash_splice(ch_hash *fhash, Mkey key, unsigned char* __return);
int ch_hash_remove(ch_hash *fhash, Mkey key);
void ch_hash_destroy(ch_hash *fhash);

#ifdef SYNC
static inline void ch_hash_lock(ch_hash *hash_) {
    pthread_mutex_lock(&(hash_->mutex));
}
static inline void ch_hash_unlock(ch_hash *hash_) {
    pthread_mutex_unlock(&(hash_->mutex));
}
#endif

#ifdef __cplusplus
}
#endif

#endif
