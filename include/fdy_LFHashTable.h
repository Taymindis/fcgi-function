#ifndef HEADER_LFHASHTABLE
#define HEADER_LFHASHTABLE

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdatomic.h>
#include <pthread.h>

typedef unsigned long int LFKey;
// typedef char* LFCKey;

typedef struct fdy_LFNode_s {
    void *val;
    LFKey key;
    atomic_flag used;
    struct fdy_LFNode_s *next;    
} fdy_LFNode;

typedef struct {
    struct fdy_LFNode_s *node_buf;
    _Atomic size_t size;
    size_t total_size;
} fdy_LFHashTable;

int fdy_LF_init(fdy_LFHashTable *lf_hashtable, size_t max_size);
int fdy_LF_put(fdy_LFHashTable *lf_hashtable, LFKey key, void *value);
void *fdy_LF_pop(fdy_LFHashTable *lf_hashtable, LFKey key);
void *fdy_LF_get(fdy_LFHashTable *lf_hashtable, LFKey key);
void fdy_LF_destroy(fdy_LFHashTable *lf_hashtable);




#ifdef __cplusplus
}
#endif

#endif