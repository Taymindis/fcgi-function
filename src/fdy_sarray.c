#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef SYNC
#include <pthread.h>
#endif
// gcc sarray.c -DSYNC=true
// same with Sarray, itis a string key based
#include "fdy_sarray.h"

static FSARRAY_MALLOC fsarray_malloc = malloc;
static FSARRAY_FREE fsarray_free = free;

void fsarray_set_allocation_fn(FSARRAY_MALLOC malloc_fun, FSARRAY_FREE free_fun) {
	fsarray_malloc = malloc_fun;
	fsarray_free = free_fun;
}

fdy_sarray* s_init_sarray(size_t length,  size_t object_size) {
	unsigned char ** s = fsarray_malloc((length + 1) * sizeof(unsigned char*));
	fdy_sarray *sarray = fsarray_malloc(sizeof(fdy_sarray));
	sarray->bucket = s;
	sarray->used = 0;
	sarray->total = length;
	sarray->type_sz = object_size;
#ifdef SYNC
	sarray->mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER; //pthread_mutex_init(&(sarray->mutex), NULL);
#endif
	return sarray;
}

void* s_newObject(fdy_sarray *sarray, SKey key) {
	if (sarray->total - sarray->used <= 0) {
		sarray->total *= 2;
		unsigned char **newBucket = fsarray_malloc(sarray->total * sizeof(unsigned char*));
		memcpy(newBucket, sarray->bucket, sarray->used * sizeof(unsigned char*));
		fsarray_free(sarray->bucket);
		sarray->bucket = newBucket;
	}
	unsigned char* __return = NULL;
	unsigned char** bucket = sarray->bucket;
	int slen = strlen(key) + 1;
	unsigned char *key_object = fsarray_malloc(slen + sarray->type_sz + 1);
	memcpy(key_object, key, slen);

	bucket[sarray->used++] = (unsigned char*) key_object;
	__return = key_object + slen;

	return __return;
}

void* s_getObject(fdy_sarray *sarray, SKey key) {
	unsigned char** bucket = sarray->bucket;
	int i, slen = strlen(key) + 1;
	unsigned char* __return = NULL;
	for (i = 0; i < sarray->used; i++, bucket++) {
		if (strcmp((SKey) *bucket, key) == 0) {
			__return = *bucket + slen;
		}
	}
	return __return;
}

int s_readObject(fdy_sarray *sarray, SKey key, void *__return) {
	unsigned char** bucket = sarray->bucket;
	int i, slen = strlen(key) + 1;
	for (i = 0; i < sarray->used; i++, bucket++) {
		if (strcmp((SKey) *bucket, key) == 0) {
			memcpy(__return, (void*)((uintptr_t)*bucket + slen), sarray->type_sz + 1);
			return 1;
		}
	}
	
	return 0;
}


int s_spliceObject(fdy_sarray *sarray, SKey key, void *__return) {
	unsigned char **src, **ptr = sarray->bucket;
	unsigned char *ret;
	int i, slen = strlen(key) + 1;
	for (i = 0; i < sarray->used; i++, ptr++) {
		// printf("%s\n", (SKey) *ptr);
		if (strcmp((SKey) *ptr, key) == 0)goto FOUND;
	}// while (ptr && *(ptr) && *(SKey*) *ptr != key) ptr++;
	return 0;
FOUND:
	src = ptr;
	ret = *ptr;
	while (*(++src)) *ptr++ = *src;
	*ptr = '\000';
	memcpy(__return, (void*)((uintptr_t)ret + slen), sarray->type_sz + 1);
	fsarray_free(ret);
	sarray->used--;
	return 1;
}

int s_removeObject(fdy_sarray *sarray, SKey key) {
	unsigned char **src, **ptr = sarray->bucket;
	unsigned char *ret;
	// int slen = strlen(key) + 1
	int i;
	for (i = 0; i < sarray->used; i++, ptr++) {
		if (strcmp((SKey) *ptr, key) == 0)goto FOUND;
	}// while (ptr && *(ptr) && *(SKey*) *ptr != key) ptr++;
	return 0;
FOUND:
	src = ptr;
	ret = *ptr;
	while (*(++src)) *ptr++ = *src;
	*ptr = '\000';
	fsarray_free(ret);
	sarray->used--;
	return 1;
}

void s_destroy(fdy_sarray *sarray) {
	unsigned char ** bucket = sarray->bucket;
	while (*(bucket)) {
		fsarray_free(*bucket++);
	}
	fsarray_free(sarray->bucket);
	fsarray_free(sarray);
}

#ifdef SYNC
void fsarray_lock(fdy_sarray *sarray) {
	pthread_mutex_lock(&(sarray->mutex));
}
void fsarray_unlock(fdy_sarray *sarray) {
	pthread_mutex_unlock(&(sarray->mutex));
}
#endif


// int main(void) {
// 	sarray  = init_sarray(3, sizeof(Object));

// 	// printf("first char array addressing at %lu\n", (sarray->bucket));
// 	// printf("10th char array addressing at %lu\n", sarray->bucket + 10 * sizeof(char*));
// 	// printf("last char array addressing at %lu\n", sarray->bucket + 50 * sizeof(char*));

// 	SKey p = "111111";
// 	Object *o = (Object*) newObject(sarray, p);
// 	o->name = "fdy_sarray: Hello, how are you?";

// 	SKey p2 = "22";
// 	Object *o2 = (Object*) newObject(sarray, p2);
// 	o2->name = "fdy_sarray: Hello, how are you 2?";

// 	SKey p3 = "123456789012";
// 	printf("%d\n", strlen(p3));
// 	Object *o3 = (Object*) newObject(sarray, p3);
// 	o3->name = "fdy_sarray: Hello, how are you 3?";

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