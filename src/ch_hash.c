#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef SYNC
#include <pthread.h>
#endif

#include "ch_hash.h"

static CH_HASH_MALLOC_FN ch_hash_malloc_fn = malloc;
static CH_HASH_FREE_FN ch_hash_free_fn = free;

void ch_hash_set_alloc_fn(CH_HASH_MALLOC_FN malloc_fun, CH_HASH_FREE_FN free_fun) {
	ch_hash_malloc_fn = malloc_fun;
	ch_hash_free_fn = free_fun;
}

ch_hash* ch_hash_init(size_t length,  size_t object_size) {
	unsigned char ** s = ch_hash_malloc_fn ((length + 1) * sizeof(unsigned char*));
    memset(s, 0x00, (length + 1) * sizeof(unsigned char*)); // Make All the footstop
	ch_hash *hash_ = ch_hash_malloc_fn(sizeof(ch_hash));
	hash_->bucket = s;
	hash_->used = 0;
	hash_->total = length;
	hash_->type_sz = object_size;
#ifdef SYNC
	hash_->mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER; //pthread_mutex_init(&(hash_->mutex), NULL);
#endif
	return hash_;
}

unsigned char* ch_hash_assign(ch_hash *hash_, Mkey key) {
	unsigned char* __return = NULL;
	if (hash_->total - hash_->used <= 0) {
		hash_->total *= 2;
		unsigned char **newBucket = ch_hash_malloc_fn(hash_->total * sizeof(unsigned char*));
		memcpy(newBucket, hash_->bucket, hash_->used * sizeof(unsigned char*));
		ch_hash_free_fn(hash_->bucket);
		hash_->bucket = newBucket;
	}
	unsigned char** bucket = hash_->bucket;

	unsigned char *key_object = ch_hash_malloc_fn(sizeof(Mkey) + hash_->type_sz + 1);

	memcpy(key_object, &key, sizeof(Mkey));
	// memcpy(key_object + sizeof(Mkey), o, hash_->type_sz);
	bucket[hash_->used++] = (unsigned char*) key_object;

	__return = key_object + sizeof(Mkey);
	return __return;
}

unsigned char* ch_hash_get(ch_hash *hash_, Mkey key) {
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

// int ch_hash_read(ch_hash *hash_, Mkey key, unsigned char* __return) {
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
// int ch_hash_splice(ch_hash *hash_, Mkey key, unsigned char* __return) {
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
// 	ch_hash_free_fn(ret);
// 	hash_->used--;
// 	return 1;
// }

int ch_hash_remove(ch_hash *hash_, Mkey key) {
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
	ch_hash_free_fn(ret);
	hash_->used--;
	return 1;
}

void ch_hash_destroy(ch_hash *hash_) {
	unsigned char ** bucket = hash_->bucket;
	while (*(bucket)) {
		ch_hash_free_fn(*bucket++);
	}
	ch_hash_free_fn(hash_->bucket);
	ch_hash_free_fn(hash_);
}

//
// typedef struct {
// 	char* name;
// } Object;
//
// int main(void) {
// 	ch_hash *hash  = ch_hash_init(3, sizeof(Object));
//
// 	Mkey p = 111111;
// 	Object *o = (Object*) ch_hash_assign(hash, p);
// 	o->name = "ch_int_array: Hello, how are you?";
//
// 	Mkey p2 = 22;
// 	Object *o2 = (Object*) ch_hash_assign(hash, p2);
// 	o2->name = "ch_int_array: Hello, how are you 2?";
//
// 	Mkey p3 = 6789012;
// 	Object *o3 = (Object*) ch_hash_assign(hash, p3);
// 	o3->name = "ch_int_array: Hello, how are you 3?";
//
// 	Object *object = (Object*) ch_hash_get(hash, 22);
//
// 	if (object)
// 		printf("%s\n", object->name);
//
//
// 	object = (Object*) ch_hash_get(hash, 111111);
//
// 	if (object)
// 		printf("%s\n", object->name);
//
// 	object = (Object*) ch_hash_get(hash, 6789012);
//
// 	if (object)
// 		printf("%s\n", object->name);
//
// 	if(ch_hash_remove(hash, 111111)) {
// 		printf("%s\n", "Successfully removed");
// 	} else {
// 		printf("%s\n", "Record Not Found");
// 	}
//
//
//
// 	ch_hash_destroy(hash);
//
// 	return 0;
// }
