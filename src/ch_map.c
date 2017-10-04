#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef SYNC
#include <pthread.h>
#endif
// gcc sarray.c -DSYNC=true
// same with Sarray, itis a string key based
#include "ch_map.h"

static CH_MAP_MALLOC_FN ch_map_malloc_fn = malloc;
static CH_MAP_FREE_FN ch_map_free_fn = free;

void ch_map_alloc_fn(CH_MAP_MALLOC_FN malloc_fun, CH_MAP_FREE_FN free_fun) {
	ch_map_malloc_fn = malloc_fun;
	ch_map_free_fn = free_fun;
}

ch_map* ch_map_init(size_t length,  size_t object_size) {
	unsigned char ** s = ch_map_malloc_fn((length + 1) * sizeof(unsigned char*));
    memset(s, 0x00, (length + 1) * sizeof(unsigned char*)); // Make the footstop
	ch_map *_map = ch_map_malloc_fn(sizeof(ch_map));
	_map->bucket = s;
	_map->used = 0;
	_map->total = length;
	_map->type_sz = object_size;
#ifdef SYNC
	_map->mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER; //pthread_mutex_init(&(sarray->mutex), NULL);
#endif
	return _map;
}

unsigned char* ch_map_assign(ch_map *map_, SKey key) {
	if (map_->total - map_->used <= 0) {
		map_->total *= 2;
		unsigned char **newBucket = ch_map_malloc_fn(map_->total * sizeof(unsigned char*));
		memcpy(newBucket, map_->bucket, map_->used * sizeof(unsigned char*));
		ch_map_free_fn(map_->bucket);
		map_->bucket = newBucket;
	}
	unsigned char* __return = NULL;
	unsigned char** bucket = map_->bucket;
	unsigned long slen = strlen(key) + 1;
	unsigned char *key_object = ch_map_malloc_fn(slen + map_->type_sz + 1);
	memcpy(key_object, key, slen);

	bucket[map_->used++] = (unsigned char*) key_object;
	__return = key_object + slen;

	return __return;
}

unsigned char* ch_map_get(ch_map *map_, SKey key) {
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

 // int ch_map_read(ch_map *map_, SKey key, unsigned char*__return) {
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


 // int ch_map_splice(ch_map *map_, SKey key, unsigned char*__return) {
 // 	unsigned char **src, **ptr = map_->bucket;
 // 	unsigned char *ret;
 //    int i;
 //    unsigned long slen = strlen(key) + 1;
 // 	for (i = 0; i < map_->used; i++, ptr++) {
 // 		// printf("%s\n", (SKey) *ptr);
 // 		if (strcmp((SKey) *ptr, key) == 0)goto FOUND;
 // 	}// while (ptr && *(ptr) && *(SKey*) *ptr != key) ptr++;
 // 	return 0;
 // FOUND:
 // 	src = ptr;
 // 	ret = *ptr;
 // 	while (*(++src)) *ptr++ = *src;
 // 	*ptr = '\000';
 // 	memcpy(__return, (unsigned char*)((uintptr_t)ret + slen), map_->type_sz + 1);
 // 	ch_map_free_fn(ret);
 // 	map_->used--;
 // 	return 1;
 // }

int ch_map_remove(ch_map *map_, SKey key) {
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
	ch_map_free_fn(ret);
	map_->used--;
	return 1;
}

void ch_map_destroy(ch_map *map_) {
	unsigned char ** bucket = map_->bucket;
	while (*(bucket)) {
		ch_map_free_fn(*bucket++);
	}
	ch_map_free_fn(map_->bucket);
	ch_map_free_fn(map_);
}

//typedef struct {
//	char* name;
//} Object;
//
//int main(void) {
//	ch_map *sarray  = ch_map_init(3, sizeof(Object));
//
//	SKey p = "111111";
//	Object *o = (Object*) ch_map_assign(sarray, p);
//	o->name = "ch_str_array: Hello, how are you?";
//
//	SKey p2 = "22";
//	Object *o2 = (Object*) ch_map_assign(sarray, p2);
//	o2->name = "ch_str_array: Hello, how are you 2?";
//
//	SKey p3 = "123456789012";
//	printf("%lu\n", strlen(p3));
//	Object *o3 = (Object*) ch_map_assign(sarray, p3);
//	o3->name = "ch_str_array: Hello, how are you 3?";
//
//	Object *object = (Object*) ch_map_get(sarray, "22");
//
//	if (object)
//		printf("%s\n", object->name);
//
//
//	object = (Object*) ch_map_get(sarray, "111111");
//
//	if (object)
//		printf("%s\n", object->name);
//
//	object = (Object*) ch_map_get(sarray, "123456789012");
//
//	if (object)
//		printf("%s\n", object->name);
//
//
//	if(ch_map_remove(sarray, "111111")) {
//		printf("%s\n", "Successfully removed");
//	}
//
//
//
//	ch_map_destroy(sarray);
//
//	return 0;
//}
