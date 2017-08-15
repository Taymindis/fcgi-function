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

struct csif_LFNode_s {
    void *val;
    LFKey key;
    atomic_flag used;
    struct csif_LFNode_s *next;    
};

typedef struct {
    struct csif_LFNode_s *node_buf;
    _Atomic size_t size;
    size_t total_size;
} csif_LFHashTable;

int csif_LF_init(csif_LFHashTable *lf_hashtable, size_t max_size);
int csif_LF_put(csif_LFHashTable *lf_hashtable, LFKey key, void *value);
void *csif_LF_pop(csif_LFHashTable *lf_hashtable, LFKey key);
void *csif_LF_get(csif_LFHashTable *lf_hashtable, LFKey key);
void csif_LF_destroy(csif_LFHashTable *lf_hashtable);




#ifdef __cplusplus
}
#endif

#endif
