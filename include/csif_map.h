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
} csif_map; // Map array

typedef void *(*CSIF_MAP_MALLOC_FN)(size_t);
typedef void (*CSIF_MAP_FREE_FN)(void*);

void csif_map_alloc_fn(CSIF_MAP_MALLOC_FN malloc_fun, CSIF_MAP_FREE_FN free_fun);

typedef char* SKey;

csif_map* csif_map_init(size_t length, size_t object_size);
unsigned char* csif_map_assign(csif_map *map_, SKey key);
unsigned char* csif_map_get(csif_map *map_, SKey key);
//int csif_map_read(csif_map *map_, SKey key, unsigned char*__return);
//int csif_map_splice(csif_map *map_, SKey key, unsigned char*__return) ;
int csif_map_remove(csif_map *map_, SKey key);
void csif_map_destroy(csif_map *map_);

#ifdef SYNC
static inline void csif_map_lock(csif_map *map_) {
    pthread_mutex_lock(&(map_->mutex));
}
static inline void csif_map_unlock(csif_map *map_) {
    pthread_mutex_unlock(&(map_->mutex));
}
#endif
#ifdef __cplusplus
}
#endif

#endif
