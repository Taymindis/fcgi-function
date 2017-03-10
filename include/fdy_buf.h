#ifndef HEADER_FBUF
#define HEADER_FBUF

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

typedef struct fbuff {
	char *data;
	size_t size;

} fdy_buff;

typedef void *(*FBUF_MALLOC)(size_t);
typedef void (*FBUF_FREE)(void*);

void fbuf_set_allocation_fn(FBUF_MALLOC malloc_fun, FBUF_FREE free_fun);
fdy_buff *fbuf_create(void);
fdy_buff *fbuf_init(const char *s);
int fbuf_add(fdy_buff *fbuf, void *buf, size_t buff_size);
int fbuf_add_(fdy_buff *fbuf, const char *s);
int fbuf_delete(fdy_buff *fbuf);

fdy_buff* fbuf_read_file(char* file_path);
int fbuf_write_file(char* out_path, fdy_buff* out_buff, int clear_buff);
int fbuf_append_file(char* out_path, fdy_buff* out_buff, int clear_buff);

#ifdef __cplusplus
}
#endif

#endif