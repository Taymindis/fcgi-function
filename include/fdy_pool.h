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
} fdy_pool;


fdy_pool* create_pool( size_t size );
void destroy_pool( fdy_pool *p );
void * falloc( fdy_pool **p, size_t size );
size_t mem_left( fdy_pool *p );
size_t blk_size( fdy_pool *p );
fdy_pool* re_create_pool( fdy_pool *curr_p);

#ifdef __cplusplus
}
#endif

#endif