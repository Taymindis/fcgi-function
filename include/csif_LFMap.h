#ifndef HEADER_LFMAP
#define HEADER_LFMAP

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdatomic.h>

typedef void *(*CSIF_LFMAP_MALLOC_FN)(size_t);
typedef void (*CSIF_LFMAP_FREE_FN)(void*);

void csif_lf_map_alloc_fn(CSIF_LFMAP_MALLOC_FN malloc_fun, CSIF_LFMAP_FREE_FN free_fun);

typedef char* LFMKey;

struct csif_LFVal_s {
    void *val;
    LFMKey key;
    int len;
    atomic_flag used;
    struct csif_LFVal_s *next;    
};

typedef struct {
    struct csif_LFVal_s *node_buf;
    _Atomic size_t size;
    size_t total_size;
} csif_LFMap;

int csif_LF_map_init(csif_LFMap *lf_map, size_t max_size);
int csif_LF_map_put(csif_LFMap *lf_map, LFMKey key, void *value);
void *csif_LF_map_pop(csif_LFMap *lf_map, LFMKey key);
void *csif_LF_map_get(csif_LFMap *lf_map, LFMKey key);
void csif_LF_map_destroy(csif_LFMap *lf_map);




#ifdef __cplusplus
}
#endif

#endif
