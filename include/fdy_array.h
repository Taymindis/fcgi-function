#ifndef HEADER_FARR
#define HEADER_FARR

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
// #include <sys/time.h>

#ifdef SYNC
#include <pthread.h>
#endif


#define ALIGN 16

#define FARR_OK 1
#define FARR_NOTOK 0	

// typedef struct farr_trash_s {
// 	void **arr;
// 	size_t sz;

// } farr_trash;

typedef struct farray {
	// farr_trash *rec_ref;
	size_t used_;
	size_t total_;

	void *_bucket_;
	size_t st_sz;  //Object/struct Size
#ifdef SYNC
	pthread_mutex_t mutex;
#endif
} fdy_arr;


typedef void *(*FARR_MALLOC)(size_t);
typedef void (*FARR_FREE)(void*);
typedef void (*ERR_P)(const char *s);


void farr_set_allocation_functions(FARR_MALLOC malloc_fun, FARR_FREE free_fun);
void farr_set_err_functions(ERR_P err_fun);

fdy_arr* new_farr(size_t total_, size_t struct_size);
void* farr_assign(fdy_arr *farr);
// int farr_push_(fdy_arr *farr, void* element);
int farr_get(fdy_arr *farr, unsigned int index, void* return_);
int farr_remove(fdy_arr *farr, unsigned int index, void* return_);
void free_farr(fdy_arr *farr);
void* farr_dup_data_arr(fdy_arr *farr);
#ifdef SYNC
void farr_lock(fdy_arr *farr);
void farr_unlock(fdy_arr *farr);
#endif


#ifdef __cplusplus
}
#endif

#endif