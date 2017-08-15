#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csif_LFMap.h"

/***
* Author: cloudleware2015@gmail.com - taymindis
* Lock Free Map, The faster travese and read, free and writeable
* For single/multithreading purposes.
* LockFree Hashtable does not care you modify share memory when other people using.
* Version 0.0.1, 15-AUG-2018
****/
void* csif_LF_map_alloc_(csif_LFMap *lf_map);
int csif_LF_map_free_(csif_LFMap *lf_map, struct csif_LFVal_s *n);

// Derived from https://gist.githubusercontent.com/mgronhol/018e17c118ccf2144744/raw/4f48a076f7628796b3311b80c524d244f02423eb/fast-str.c
int fast_compare( const char *ptr0, const char *ptr1, size_t len );


static CSIF_LFMAP_MALLOC_FN csif_lf_map_malloc_fn = malloc;
static CSIF_LFMAP_FREE_FN csif_lf_map_free_fn = free;

void csif_lf_map_alloc_fn(CSIF_LFMAP_MALLOC_FN malloc_fun, CSIF_LFMAP_FREE_FN free_fun) {
	csif_lf_map_malloc_fn = malloc_fun;
	csif_lf_map_free_fn = free_fun;
}

void* csif_LF_map_alloc_(csif_LFMap *lf_map) {

	struct csif_LFVal_s * return_ = lf_map->node_buf;

RETRY:
	while (atomic_flag_test_and_set_explicit(&return_->used, memory_order_acquire)) {
		return_++;
	}

	// To identify if LFNode is NULL but still got slot, need to retry to find a slot, it might happen when reach the last slot
	if (return_ == NULL && atomic_load(&lf_map->size) < lf_map->total_size) goto RETRY;

	return return_;
}

int csif_LF_map_free_(csif_LFMap *lf_map, struct csif_LFVal_s *n) {
	if (n->key) {
		csif_lf_map_free_fn(n->key);
		n->key = 0;
	}

	n->val = NULL;

	// To make it unused slot
	atomic_flag_clear_explicit(&n->used, memory_order_release);

	return 1;
}


/*API Endpoint*/

int csif_LF_map_init(csif_LFMap *lf_map, size_t max_size) {

	lf_map->total_size = max_size;
	max_size += 3; // give some padding

	/** Pre-allocate all nodes **/
	lf_map->node_buf = csif_lf_map_malloc_fn(max_size * sizeof(struct csif_LFVal_s));

	if (lf_map->node_buf == NULL)
		return 0;

	for (size_t i = 0; i < max_size - 1; i++) {
		lf_map->node_buf[i].key = 0;
		lf_map->node_buf[i].val = NULL;
		lf_map->node_buf[i].next = lf_map->node_buf + i + 1;
		lf_map->node_buf[i].used = (atomic_flag)ATOMIC_FLAG_INIT;
		// atomic_flag_clear(&lf_map->node_buf[i].used);

	}
	//For Last Node
	lf_map->node_buf[max_size - 1].next = NULL;

	// Atomic Init
	lf_map->size = ATOMIC_VAR_INIT(0);

	return 1;
}

int csif_LF_map_put(csif_LFMap *lf_map, LFMKey key_, void *value) {
	int keyLen = strlen(key_);
	// for (keyLen=0;key_[keyLen]!='\0';keyLen++);

	// We add first to prevent any error;
	if (atomic_fetch_add(&lf_map->size, 1) < lf_map->total_size) {
		struct csif_LFVal_s *node = csif_LF_map_alloc_(lf_map);
		if (node) {
			node->val = value;
			node->key = csif_lf_map_malloc_fn((keyLen + 1) * sizeof(char));
			memcpy(node->key, key_, keyLen + 1);
			node->len = keyLen;
			return 1;
		} else {
			// If can't add more, rollback
			atomic_fetch_sub(&lf_map->size, 1);
		}
	}
	// If can't add more, rollback
	atomic_fetch_sub(&lf_map->size, 1);
	return 0;
}

void *csif_LF_map_pop(csif_LFMap *lf_map, LFMKey key_) {
	void * return_ = NULL;
	int keyLen = strlen(key_);

	struct csif_LFVal_s *buffer = lf_map->node_buf;

	do {
		if (buffer->key && fast_compare(key_, buffer->key, keyLen) == 0) goto SUCCESS;
	} while ((buffer = buffer->next) != NULL);
SUCCESS:
	return_ = buffer->val;
	if (csif_LF_map_free_(lf_map, buffer)) {
		atomic_fetch_sub(&lf_map->size, 1);
	}
	return return_;
}

void *csif_LF_map_get(csif_LFMap *lf_map, LFMKey key_) {
	void * return_ = NULL;
	int keyLen = strlen(key_);

	struct csif_LFVal_s *buffer = lf_map->node_buf;

	do {
		if (buffer->key && fast_compare(key_, buffer->key, keyLen) == 0) goto SUCCESS;
	} while ((buffer = buffer->next) != NULL);
	return return_;
SUCCESS:
	return_ = buffer->val;
	return return_;
}

// need to implement csif_LF_read(csif_LFMap *lf_map, LFMKey key)

void csif_LF_map_destroy(csif_LFMap *lf_map) {
	free(lf_map->node_buf);
}

// Derived from https://gist.githubusercontent.com/mgronhol/018e17c118ccf2144744/raw/4f48a076f7628796b3311b80c524d244f02423eb/fast-str.c
int fast_compare( const char *ptr0, const char *ptr1, size_t len ) {
	int fast = len / sizeof(size_t) + 1;
	int offset = (fast - 1) * sizeof(size_t);
	int current_block = 0;

	if ( len <= sizeof(size_t)) { fast = 0; }


	size_t *lptr0 = (size_t*)ptr0;
	size_t *lptr1 = (size_t*)ptr1;

	while ( current_block < fast ) {
		if ( (lptr0[current_block] ^ lptr1[current_block] )) {
			int pos;
			for (pos = current_block * sizeof(size_t); pos < len ; ++pos ) {
				if ( (ptr0[pos] ^ ptr1[pos]) || (ptr0[pos] == 0) || (ptr1[pos] == 0) ) {
					return  (int)((unsigned char)ptr0[pos] - (unsigned char)ptr1[pos]);
				}
			}
		}

		++current_block;
	}

	while ( len > offset ) {
		if ( (ptr0[offset] ^ ptr1[offset] )) {
			return (int)((unsigned char)ptr0[offset] - (unsigned char)ptr1[offset]);
		}
		++offset;
	}

	return 0;
}

/** Test SCOPE **/
// typedef struct {
// 	char* name;
// } Object;

// int main(void) {
// 	csif_LFMap lf_map;

// 	if (csif_LF_map_init(&lf_map, 3)) {


// 		Object *o = (Object*)malloc(sizeof(Object));
// 		o->name = "csif_str_array: Hello, how are you?";
// 		csif_LF_map_put(&lf_map, "12 3", o);


// 		Object *o2 = (Object*)malloc(sizeof(Object));
// 		o2->name = "csif_str_array: Hello, how are you 2?";
// 		csif_LF_map_put(&lf_map, "1321", o2);

// 		Object *o3 = (Object*)malloc(sizeof(Object));
// 		o3->name = "csif_str_array: Hello, how are you 3?";
// 		csif_LF_map_put(&lf_map, "sx", o3);

// 		Object *object = (Object*) csif_LF_map_pop(&lf_map, "sx");

// 		if (object) {
// 			printf("%s\n", object->name);
// 			free(object);
// 		}


// 		object = (Object*) csif_LF_map_pop(&lf_map, "12 3");

// 		if (object) {
// 			printf("%s\n", object->name);
// 			free(object);
// 		}

// 		object = (Object*) csif_LF_map_pop(&lf_map, "1321");

// 		if (object) {
// 			printf("%s\n", object->name);
// 			free(object);
// 		}



// 		csif_LF_map_destroy(&lf_map);
// 	}

// 	return 0;
// }

