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
} ffunc_map; // Map array

typedef void *(*CH_MAP_MALLOC_FN)(size_t);
typedef void (*CH_MAP_FREE_FN)(void*);

void ffunc_map_alloc_fn(CH_MAP_MALLOC_FN malloc_fun, CH_MAP_FREE_FN free_fun);

typedef char* SKey;

ffunc_map* ffunc_map_init(size_t length, size_t object_size);
unsigned char* ffunc_map_assign(ffunc_map *map_, SKey key);
unsigned char* ffunc_map_get(ffunc_map *map_, SKey key);
//int ffunc_map_read(ffunc_map *map_, SKey key, unsigned char*__return);
//int ffunc_map_splice(ffunc_map *map_, SKey key, unsigned char*__return) ;
int ffunc_map_remove(ffunc_map *map_, SKey key);
void ffunc_map_destroy(ffunc_map *map_);

#ifdef SYNC
static inline void ffunc_map_lock(ffunc_map *map_) {
    pthread_mutex_lock(&(map_->mutex));
}
static inline void ffunc_map_unlock(ffunc_map *map_) {
    pthread_mutex_unlock(&(map_->mutex));
}
#endif
#ifdef __cplusplus
}
#endif

#endif
