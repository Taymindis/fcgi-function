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

} ch_buf;

typedef void *(*CH_BUF_MALLOC_FN)(size_t);
typedef void (*CH_BUF_FREE_FN)(void*);

void ch_buf_set_alloc_fn(CH_BUF_MALLOC_FN malloc_fun, CH_BUF_FREE_FN free_fun);
ch_buf *ch_buf_create(void);
ch_buf *ch_buf_init(const char *s);
int ch_buf_add(ch_buf *fbuf, void *buf, size_t buff_size);
int ch_buf_add_(ch_buf *fbuf, const char *s);
int ch_buf_delete(ch_buf *fbuf);

ch_buf* ch_buf_read_file(char* file_path);
int ch_buf_write_file(char* out_path, ch_buf* out_buff, int clear_buff);
int ch_buf_append_file(char* out_path, ch_buf* out_buff, int clear_buff);

#ifdef __cplusplus
}
#endif

#endif
