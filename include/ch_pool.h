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
} ch_pool;


ch_pool* create_pool( size_t size );
void destroy_pool( ch_pool *p );
void * falloc( ch_pool **p, size_t size );
size_t mem_left( ch_pool *p );
size_t blk_size( ch_pool *p );
ch_pool* re_create_pool( ch_pool *curr_p);

#ifdef __cplusplus
}
#endif

#endif
