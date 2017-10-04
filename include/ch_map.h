#ifndef _HEADER_FHASH_STR
#define _HEADER_FHASH_STR

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <pthread.h>

typedef struct {
	unsigned char** bucket;
	int used;
	unsigned long total;
	size_t type_sz;
#ifdef SYNC
	pthread_mutex_t mutex;
#endif
} ch_map; // Map array

typedef void *(*CH_MAP_MALLOC_FN)(size_t);
typedef void (*CH_MAP_FREE_FN)(void*);

void ch_map_alloc_fn(CH_MAP_MALLOC_FN malloc_fun, CH_MAP_FREE_FN free_fun);

typedef char* SKey;

ch_map* ch_map_init(size_t length, size_t object_size);
unsigned char* ch_map_assign(ch_map *map_, SKey key);
unsigned char* ch_map_get(ch_map *map_, SKey key);
//int ch_map_read(ch_map *map_, SKey key, unsigned char*__return);
//int ch_map_splice(ch_map *map_, SKey key, unsigned char*__return) ;
int ch_map_remove(ch_map *map_, SKey key);
void ch_map_destroy(ch_map *map_);

#ifdef SYNC
static inline void ch_map_lock(ch_map *map_) {
    pthread_mutex_lock(&(map_->mutex));
}
static inline void ch_map_unlock(ch_map *map_) {
    pthread_mutex_unlock(&(map_->mutex));
}
#endif
#ifdef __cplusplus
}
#endif

#endif
