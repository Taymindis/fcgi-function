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
	unsigned char *data;
	size_t size;

} ffunc_buf;

typedef void *(*CH_BUF_MALLOC_FN)(size_t);
typedef void (*CH_BUF_FREE_FN)(void*);

void ffunc_buf_set_alloc_fn(CH_BUF_MALLOC_FN malloc_fun, CH_BUF_FREE_FN free_fun);
ffunc_buf *ffunc_buf_create(void);
ffunc_buf *ffunc_buf_init(const char *s);
int ffunc_buf_add(ffunc_buf *fbuf, void *buf, size_t buff_size);
int ffunc_buf_add_(ffunc_buf *fbuf, const char *s);
int ffunc_buf_delete(ffunc_buf *fbuf);

ffunc_buf* ffunc_buf_read_file(char* file_path);
int ffunc_buf_write_file(char* out_path, ffunc_buf* out_buff, int clear_buff);
int ffunc_buf_append_file(char* out_path, ffunc_buf* out_buff, int clear_buff);

#ifdef __cplusplus
}
#endif

#endif
