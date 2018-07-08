#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef SYNC
#include <pthread.h>
#endif

#include "ffunc_hash.h"

static CH_HASH_MALLOC_FN ffunc_hash_malloc_fn = malloc;
static CH_HASH_FREE_FN ffunc_hash_free_fn = free;

void ffunc_hash_set_alloc_fn(CH_HASH_MALLOC_FN malloc_fun, CH_HASH_FREE_FN free_fun) {
	ffunc_hash_malloc_fn = malloc_fun;
	ffunc_hash_free_fn = free_fun;
}

ffunc_hash* ffunc_hash_init(size_t length,  size_t object_size) {
	unsigned char ** s = ffunc_hash_malloc_fn ((length + 1) * sizeof(unsigned char*));
    memset(s, 0x00, (length + 1) * sizeof(unsigned char*)); // Make All the footstop
	ffunc_hash *hash_ = ffunc_hash_malloc_fn(sizeof(ffunc_hash));
	hash_->bucket = s;
	hash_->used = 0;
	hash_->total = length;
	hash_->type_sz = object_size;
#ifdef SYNC
	hash_->mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER; //pthread_mutex_init(&(hash_->mutex), NULL);
#endif
	return hash_;
}

unsigned char* ffunc_hash_assign(ffunc_hash *hash_, Mkey key) {
	unsigned char* __return = NULL;
	if (hash_->total - hash_->used <= 0) {
		hash_->total *= 2;
		unsigned char **newBucket = ffunc_hash_malloc_fn(hash_->total * sizeof(unsigned char*));
		memcpy(newBucket, hash_->bucket, hash_->used * sizeof(unsigned char*));
		ffunc_hash_free_fn(hash_->bucket);
		hash_->bucket = newBucket;
	}
	unsigned char** bucket = hash_->bucket;

	unsigned char *key_object = ffunc_hash_malloc_fn(sizeof(Mkey) + hash_->type_sz + 1);

	memcpy(key_object, &key, sizeof(Mkey));
	// memcpy(key_object + sizeof(Mkey), o, hash_->type_sz);
	bucket[hash_->used++] = (unsigned char*) key_object;

	__return = key_object + sizeof(Mkey);
	return __return;
}

unsigned char* ffunc_hash_get(ffunc_hash *hash_, Mkey key) {
	unsigned char** bucket = hash_->bucket;
	unsigned char* __return = NULL;
	int i;
	for (i = 0; i < hash_->used; i++, bucket++) {
		if (* (Mkey*) *bucket == key) {
			__return = *bucket + sizeof(Mkey);
		}
	}
	return __return;
}

// int ffunc_hash_read(ffunc_hash *hash_, Mkey key, unsigned char* __return) {
// 	unsigned char** bucket = hash_->bucket;
// 	int i;
// 	for (i = 0; i < hash_->used; i++, bucket++) {
// 		if (* (Mkey*) *bucket == key) {
// 			memcpy(__return,  (unsigned char*)((uintptr_t) *bucket + sizeof(Mkey)), hash_->type_sz + 1);
// 			return 1;
// 		}
// 	}
// 	return 0;
// }
//
//
// int ffunc_hash_splice(ffunc_hash *hash_, Mkey key, unsigned char* __return) {
// 	unsigned char **src, **ptr = hash_->bucket;
// 	unsigned char *ret;
// 	int i;
// 	for (i = 0; i < hash_->used; i++, ptr++) {
// 		if (* (Mkey*) *ptr == key)goto FOUND;
// 	}// while (ptr && *(ptr) && *(Mkey*) *ptr != key) ptr++;
// 	return 0;
// FOUND:
// 	src = ptr;
// 	ret = *ptr;
// 	while (*(++src)) *ptr++ = *src;
// 	*ptr = '\000';
// 	memcpy(__return, (unsigned char*)((uintptr_t)ret + sizeof(Mkey)), hash_->type_sz + 1);
// 	ffunc_hash_free_fn(ret);
// 	hash_->used--;
// 	return 1;
// }

int ffunc_hash_remove(ffunc_hash *hash_, Mkey key) {
	unsigned char **src, **ptr = hash_->bucket;
	unsigned char *ret;
	int i;
	for (i = 0; i < hash_->used; i++, ptr++) {
		if (* (Mkey*) *ptr == key)goto FOUND;
	}// while (ptr && *(ptr) && *(Mkey*) *ptr != key) ptr++;
	return 0;
FOUND:
	src = ptr;
	ret = *ptr;
	while (*(++src)) *ptr++ = *src;
	*ptr = '\000';
	ffunc_hash_free_fn(ret);
	hash_->used--;
	return 1;
}

void ffunc_hash_destroy(ffunc_hash *hash_) {
	unsigned char ** bucket = hash_->bucket;
	while (*(bucket)) {
		ffunc_hash_free_fn(*bucket++);
	}
	ffunc_hash_free_fn(hash_->bucket);
	ffunc_hash_free_fn(hash_);
}

//
// typedef struct {
// 	char* name;
// } Object;
//
// int main(void) {
// 	ffunc_hash *hash  = ffunc_hash_init(3, sizeof(Object));
//
// 	Mkey p = 111111;
// 	Object *o = (Object*) ffunc_hash_assign(hash, p);
// 	o->name = "ffunc_int_array: Hello, how are you?";
//
// 	Mkey p2 = 22;
// 	Object *o2 = (Object*) ffunc_hash_assign(hash, p2);
// 	o2->name = "ffunc_int_array: Hello, how are you 2?";
//
// 	Mkey p3 = 6789012;
// 	Object *o3 = (Object*) ffunc_hash_assign(hash, p3);
// 	o3->name = "ffunc_int_array: Hello, how are you 3?";
//
// 	Object *object = (Object*) ffunc_hash_get(hash, 22);
//
// 	if (object)
// 		ffunc_print("%s\n", object->name);
//
//
// 	object = (Object*) ffunc_hash_get(hash, 111111);
//
// 	if (object)
// 		ffunc_print("%s\n", object->name);
//
// 	object = (Object*) ffunc_hash_get(hash, 6789012);
//
// 	if (object)
// 		ffunc_print("%s\n", object->name);
//
// 	if(ffunc_hash_remove(hash, 111111)) {
// 		ffunc_print("%s\n", "Successfully removed");
// 	} else {
// 		ffunc_print("%s\n", "Record Not Found");
// 	}
//
//
//
// 	ffunc_hash_destroy(hash);
//
// 	return 0;
// }
