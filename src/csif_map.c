#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef SYNC
#include <pthread.h>
#endif
// gcc sarray.c -DSYNC=true
// same with Sarray, itis a string key based
#include "csif_map.h"

static CSIF_MAP_MALLOC_FN csif_map_malloc_fn = malloc;
static CSIF_MAP_FREE_FN csif_map_free_fn = free;

void csif_map_alloc_fn(CSIF_MAP_MALLOC_FN malloc_fun, CSIF_MAP_FREE_FN free_fun) {
	csif_map_malloc_fn = malloc_fun;
	csif_map_free_fn = free_fun;
}

csif_map* csif_map_init(size_t length,  size_t object_size) {
	unsigned char ** s = csif_map_malloc_fn((length + 1) * sizeof(unsigned char*));
	csif_map *_map = csif_map_malloc_fn(sizeof(csif_map));
	_map->bucket = s;
	_map->used = 0;
	_map->total = length;
	_map->type_sz = object_size;
#ifdef SYNC
	_map->mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER; //pthread_mutex_init(&(sarray->mutex), NULL);
#endif
	return _map;
}

void* csif_map_assign(csif_map *map_, SKey key) {
	if (map_->total - map_->used <= 0) {
		map_->total *= 2;
		unsigned char **newBucket = csif_map_malloc_fn(map_->total * sizeof(unsigned char*));
		memcpy(newBucket, map_->bucket, map_->used * sizeof(unsigned char*));
		csif_map_free_fn(map_->bucket);
		map_->bucket = newBucket;
	}
	unsigned char* __return = NULL;
	unsigned char** bucket = map_->bucket;
	int slen = strlen(key) + 1;
	unsigned char *key_object = csif_map_malloc_fn(slen + map_->type_sz + 1);
	memcpy(key_object, key, slen);

	bucket[map_->used++] = (unsigned char*) key_object;
	__return = key_object + slen;

	return __return;
}

void* csif_map_get(csif_map *map_, SKey key) {
	unsigned char** bucket = map_->bucket;
	int i, slen = strlen(key) + 1;
	unsigned char* __return = NULL;
	for (i = 0; i < map_->used; i++, bucket++) {
		if (strcmp((SKey) *bucket, key) == 0) {
			__return = *bucket + slen;
		}
	}
	return __return;
}

int csif_map_read(csif_map *map_, SKey key, void *__return) {
	unsigned char** bucket = map_->bucket;
	int i, slen = strlen(key) + 1;
	for (i = 0; i < map_->used; i++, bucket++) {
		if (strcmp((SKey) *bucket, key) == 0) {
			memcpy(__return, (void*)((uintptr_t)*bucket + slen), map_->type_sz + 1);
			return 1;
		}
	}
	
	return 0;
}


int csif_map_splice(csif_map *map_, SKey key, void *__return) {
	unsigned char **src, **ptr = map_->bucket;
	unsigned char *ret;
	int i, slen = strlen(key) + 1;
	for (i = 0; i < map_->used; i++, ptr++) {
		// printf("%s\n", (SKey) *ptr);
		if (strcmp((SKey) *ptr, key) == 0)goto FOUND;
	}// while (ptr && *(ptr) && *(SKey*) *ptr != key) ptr++;
	return 0;
FOUND:
	src = ptr;
	ret = *ptr;
	while (*(++src)) *ptr++ = *src;
	*ptr = '\000';
	memcpy(__return, (void*)((uintptr_t)ret + slen), map_->type_sz + 1);
	csif_map_free_fn(ret);
	map_->used--;
	return 1;
}

int csif_map_remove(csif_map *map_, SKey key) {
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
	csif_map_free_fn(ret);
	map_->used--;
	return 1;
}

void csif_map_destroy(csif_map *map_) {
	unsigned char ** bucket = map_->bucket;
	while (*(bucket)) {
		csif_map_free_fn(*bucket++);
	}
	csif_map_free_fn(map_->bucket);
	csif_map_free_fn(map_);
}

// int main(void) {
// 	sarray  = init_sarray(3, sizeof(Object));

// 	// printf("first char array addressing at %lu\n", (sarray->bucket));
// 	// printf("10th char array addressing at %lu\n", sarray->bucket + 10 * sizeof(char*));
// 	// printf("last char array addressing at %lu\n", sarray->bucket + 50 * sizeof(char*));

// 	SKey p = "111111";
// 	Object *o = (Object*) newObject(sarray, p);
// 	o->name = "csif_str_array: Hello, how are you?";

// 	SKey p2 = "22";
// 	Object *o2 = (Object*) newObject(sarray, p2);
// 	o2->name = "csif_str_array: Hello, how are you 2?";

// 	SKey p3 = "123456789012";
// 	printf("%d\n", strlen(p3));
// 	Object *o3 = (Object*) newObject(sarray, p3);
// 	o3->name = "csif_str_array: Hello, how are you 3?";

// 	// if (p == 1231231274891274124) {
// 	// 	printf("%s\n", "true");
// 	// }

// 	Object *object = (Object*) getObject(sarray, "22");

// 	if (object)
// 		printf("%s\n", object->name);


// 	object = (Object*) spliceObject(sarray, "111111");

// 	if (object)
// 		printf("%s\n", object->name);

// 	object = (Object*) spliceObject(sarray, "123456789012");

// 	if (object) //this is not found ady.
// 		printf("%s\n", object->name);
// // free(o2);
// 	destroy(sarray);

// 	return 0;
// }
