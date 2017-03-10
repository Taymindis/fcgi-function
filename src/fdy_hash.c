#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef SYNC
#include <pthread.h>
#endif

#include "fdy_hash.h"

static FHASH_MALLOC fhash_malloc = malloc;
static FHASH_FREE fhash_free = free;

void fhash_set_allocation_fn(FHASH_MALLOC malloc_fun, FHASH_FREE free_fun) {
	fhash_malloc = malloc_fun;
	fhash_free = free_fun;
}

fdy_hash* init_fhash(size_t length,  size_t object_size) {
	unsigned char ** s = fhash_malloc ((length + 1) * sizeof(unsigned char*) );
	fdy_hash *fhash = fhash_malloc(sizeof(fdy_hash));
	fhash->bucket = s;
	fhash->used = 0;
	fhash->total = length;
	fhash->type_sz = object_size;
#ifdef SYNC
	fhash->mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER; //pthread_mutex_init(&(fhash->mutex), NULL);
#endif
	return fhash;
}

void* newObject(fdy_hash *fhash, Mkey key) {
	unsigned char* __return = NULL;
	if (fhash->total - fhash->used <= 0) {
		fhash->total *= 2;
		unsigned char **newBucket = fhash_malloc(fhash->total * sizeof(unsigned char*));
		memcpy(newBucket, fhash->bucket, fhash->used * sizeof(unsigned char*));
		fhash_free(fhash->bucket);
		fhash->bucket = newBucket;
	}
	unsigned char** bucket = fhash->bucket;

	unsigned char *key_object = fhash_malloc(sizeof(Mkey) + fhash->type_sz + 1);

	memcpy(key_object, &key, sizeof(Mkey));
	// memcpy(key_object + sizeof(Mkey), o, fhash->type_sz);
	bucket[fhash->used++] = (unsigned char*) key_object;

	__return = key_object + sizeof(Mkey);
	return __return;
}

void* getObject(fdy_hash *fhash, Mkey key) {
	unsigned char** bucket = fhash->bucket;
	unsigned char* __return = NULL;
	int i;
	for (i = 0; i < fhash->used; i++, bucket++) {
		if (* (Mkey*) *bucket == key) {
			__return = *bucket + sizeof(Mkey);
		}
	}
	return __return;
}

int readObject(fdy_hash *fhash, Mkey key, void* __return) {
	unsigned char** bucket = fhash->bucket;
	int i;
	for (i = 0; i < fhash->used; i++, bucket++) {
		if (* (Mkey*) *bucket == key) {
			memcpy(__return,  (void*)((uintptr_t) *bucket + sizeof(Mkey)), fhash->type_sz + 1);
			return 1;
		}
	}
	return 0;
}


int spliceObject(fdy_hash *fhash, Mkey key, void* __return) {
	unsigned char **src, **ptr = fhash->bucket;
	unsigned char *ret;
	int i;
	for (i = 0; i < fhash->used; i++, ptr++) {
		if (* (Mkey*) *ptr == key)goto FOUND;
	}// while (ptr && *(ptr) && *(Mkey*) *ptr != key) ptr++;
	return 0;
FOUND:
	src = ptr;
	ret = *ptr;
	while (*(++src)) *ptr++ = *src;
	*ptr = '\000';
	memcpy(__return, (void*)((uintptr_t)ret + sizeof(Mkey)), fhash->type_sz + 1);
	fhash_free(ret);
	fhash->used--;
	return 1;
}

int removeObject(fdy_hash *fhash, Mkey key) {
	unsigned char **src, **ptr = fhash->bucket;
	unsigned char *ret;
	int i;
	for (i = 0; i < fhash->used; i++, ptr++) {
		if (* (Mkey*) *ptr == key)goto FOUND;
	}// while (ptr && *(ptr) && *(Mkey*) *ptr != key) ptr++;
	return 0;
FOUND:
	src = ptr;
	ret = *ptr;
	while (*(++src)) *ptr++ = *src;
	*ptr = '\000';
	fhash_free(ret);
	fhash->used--;
	return 1;
}

void destroy(fdy_hash *fhash) {
	unsigned char ** bucket = fhash->bucket;
	while (*(bucket)) {
		fhash_free(*bucket++);
	}
	fhash_free(bucket);
	fhash_free(fhash);
}


#ifdef SYNC
void fhash_lock(fdy_hash *fhash) {
	pthread_mutex_lock(&(fhash->mutex));
}
void fhash_unlock(fdy_hash *fhash) {
	pthread_mutex_unlock(&(fhash->mutex));
}
#endif