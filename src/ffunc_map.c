#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef SYNC
#include <pthread.h>
#endif
// gcc sarray.c -DSYNC=true
// same with Sarray, itis a string key based
#include "ffunc_map.h"

static CH_MAP_MALLOC_FN ffunc_map_malloc_fn = malloc;
static CH_MAP_FREE_FN ffunc_map_free_fn = free;

void ffunc_map_alloc_fn(CH_MAP_MALLOC_FN malloc_fun, CH_MAP_FREE_FN free_fun) {
	ffunc_map_malloc_fn = malloc_fun;
	ffunc_map_free_fn = free_fun;
}

ffunc_map* ffunc_map_init(size_t length,  size_t object_size) {
	unsigned char ** s = ffunc_map_malloc_fn((length + 1) * sizeof(unsigned char*));
    memset(s, 0x00, (length + 1) * sizeof(unsigned char*)); // Make the footstop
	ffunc_map *_map = ffunc_map_malloc_fn(sizeof(ffunc_map));
	_map->bucket = s;
	_map->used = 0;
	_map->total = length;
	_map->type_sz = object_size;
#ifdef SYNC
	_map->mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER; //pthread_mutex_init(&(sarray->mutex), NULL);
#endif
	return _map;
}

unsigned char* ffunc_map_assign(ffunc_map *map_, SKey key) {
	if (map_->total - map_->used <= 0) {
		map_->total *= 2;
		unsigned char **newBucket = ffunc_map_malloc_fn(map_->total * sizeof(unsigned char*));
		memcpy(newBucket, map_->bucket, map_->used * sizeof(unsigned char*));
		ffunc_map_free_fn(map_->bucket);
		map_->bucket = newBucket;
	}
	unsigned char* __return = NULL;
	unsigned char** bucket = map_->bucket;
	unsigned long slen = strlen(key) + 1;
	unsigned char *key_object = ffunc_map_malloc_fn(slen + map_->type_sz + 1);
	memcpy(key_object, key, slen);

	bucket[map_->used++] = (unsigned char*) key_object;
	__return = key_object + slen;

	return __return;
}

unsigned char* ffunc_map_get(ffunc_map *map_, SKey key) {
	unsigned char** bucket = map_->bucket;
    int i;
    unsigned long slen = strlen(key) + 1;
	unsigned char* __return = NULL;
	for (i = 0; i < map_->used; i++, bucket++) {
		if (strcmp((SKey) *bucket, key) == 0) {
			__return = *bucket + slen;
		}
	}
	return __return;
}

 // int ffunc_map_read(ffunc_map *map_, SKey key, unsigned char*__return) {
 // 	unsigned char** bucket = map_->bucket;
 //    int i;
 //    unsigned long slen = strlen(key) + 1;
 // 	for (i = 0; i < map_->used; i++, bucket++) {
 // 		if (strcmp((SKey) *bucket, key) == 0) {
 // 			memcpy(__return, (unsigned char*)((uintptr_t)*bucket + slen), map_->type_sz + 1);
 // 			return 1;
 // 		}
 // 	}
	
 // 	return 0;
 // }


 // int ffunc_map_splice(ffunc_map *map_, SKey key, unsigned char*__return) {
 // 	unsigned char **src, **ptr = map_->bucket;
 // 	unsigned char *ret;
 //    int i;
 //    unsigned long slen = strlen(key) + 1;
 // 	for (i = 0; i < map_->used; i++, ptr++) {
 // 		// ffunc_print("%s\n", (SKey) *ptr);
 // 		if (strcmp((SKey) *ptr, key) == 0)goto FOUND;
 // 	}// while (ptr && *(ptr) && *(SKey*) *ptr != key) ptr++;
 // 	return 0;
 // FOUND:
 // 	src = ptr;
 // 	ret = *ptr;
 // 	while (*(++src)) *ptr++ = *src;
 // 	*ptr = '\000';
 // 	memcpy(__return, (unsigned char*)((uintptr_t)ret + slen), map_->type_sz + 1);
 // 	ffunc_map_free_fn(ret);
 // 	map_->used--;
 // 	return 1;
 // }

int ffunc_map_remove(ffunc_map *map_, SKey key) {
	unsigned char **src, **ptr = map_->bucket;
	unsigned char *ret;
	// int slen = strlen(key) + 1
	int i;
	for (i = 0; i < map_->used; i++, ptr++) {
		if (strcmp((SKey) *ptr, key) == 0)goto FOUND;
	}// while (ptr && *(ptr) && *(SKey*) *ptr != key) ptr++;
	return 0;
FOUND:
	src = ptr;
	ret = *ptr;
	while (*(++src)) *ptr++ = *src;
	*ptr = '\000';
	ffunc_map_free_fn(ret);
	map_->used--;
	return 1;
}

void ffunc_map_destroy(ffunc_map *map_) {
	unsigned char ** bucket = map_->bucket;
	while (*(bucket)) {
		ffunc_map_free_fn(*bucket++);
	}
	ffunc_map_free_fn(map_->bucket);
	ffunc_map_free_fn(map_);
}

//typedef struct {
//	char* name;
//} Object;
//
//int main(void) {
//	ffunc_map *sarray  = ffunc_map_init(3, sizeof(Object));
//
//	SKey p = "111111";
//	Object *o = (Object*) ffunc_map_assign(sarray, p);
//	o->name = "ffunc_str_array: Hello, how are you?";
//
//	SKey p2 = "22";
//	Object *o2 = (Object*) ffunc_map_assign(sarray, p2);
//	o2->name = "ffunc_str_array: Hello, how are you 2?";
//
//	SKey p3 = "123456789012";
//	ffunc_print("%lu\n", strlen(p3));
//	Object *o3 = (Object*) ffunc_map_assign(sarray, p3);
//	o3->name = "ffunc_str_array: Hello, how are you 3?";
//
//	Object *object = (Object*) ffunc_map_get(sarray, "22");
//
//	if (object)
//		ffunc_print("%s\n", object->name);
//
//
//	object = (Object*) ffunc_map_get(sarray, "111111");
//
//	if (object)
//		ffunc_print("%s\n", object->name);
//
//	object = (Object*) ffunc_map_get(sarray, "123456789012");
//
//	if (object)
//		ffunc_print("%s\n", object->name);
//
//
//	if(ffunc_map_remove(sarray, "111111")) {
//		ffunc_print("%s\n", "Successfully removed");
//	}
//
//
//
//	ffunc_map_destroy(sarray);
//
//	return 0;
//}
