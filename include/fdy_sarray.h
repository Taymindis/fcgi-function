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
	int total;
	size_t type_sz;
#ifdef SYNC
	pthread_mutex_t mutex;
#endif
} fdy_sarray; // Map array


// typedef struct
// {
// 	char *name;
// } Object;

typedef void *(*FSARRAY_MALLOC)(size_t);
typedef void (*FSARRAY_FREE)(void*);

void fsarray_set_allocation_fn(FSARRAY_MALLOC malloc_fun, FSARRAY_FREE free_fun);

typedef char* SKey;

fdy_sarray* s_init_sarray(size_t length, size_t object_size);
void* s_newObject(fdy_sarray *sarray, SKey key);
void* s_getObject(fdy_sarray *sarray, SKey key);
int s_readObject(fdy_sarray *sarray, SKey key, void *__return);
int s_spliceObject(fdy_sarray *sarray, SKey key, void *__return) ;
int s_removeObject(fdy_sarray *sarray, SKey key);
void s_destroy(fdy_sarray *sarray);

#ifdef SYNC
void fsarray_lock(fdy_sarray *sarray);
void fsarray_unlock(fdy_sarray *sarray);
#endif

#ifdef __cplusplus
}
#endif

#endif