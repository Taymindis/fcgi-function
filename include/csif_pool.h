#ifndef HEADER_FMEM
#define HEADER_FMEM

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#ifndef __APPLE__	
#include <malloc.h>
#endif
#define DEFAULT_BLK_SZ 2048
#define ALIGNMENT   16

typedef struct pool {
	void *next,
	     *end;
} csif_pool;


csif_pool* create_pool( size_t size );
void destroy_pool( csif_pool *p );
void * falloc( csif_pool **p, size_t size );
size_t mem_left( csif_pool *p );
size_t blk_size( csif_pool *p );
csif_pool* re_create_pool( csif_pool *curr_p);

#ifdef __cplusplus
}
#endif

#endif
