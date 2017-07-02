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

} csif_buf;

typedef void *(*CSIF_BUF_MALLOC_FN)(size_t);
typedef void (*CSIF_BUF_FREE_FN)(void*);

void csif_buf_set_alloc_fn(CSIF_BUF_MALLOC_FN malloc_fun, CSIF_BUF_FREE_FN free_fun);
csif_buf *csif_buf_create(void);
csif_buf *csif_buf_init(const unsigned char *s);
int csif_buf_add(csif_buf *fbuf, void *buf, size_t buff_size);
int csif_buf_add_(csif_buf *fbuf, const unsigned char *s);
int csif_buf_delete(csif_buf *fbuf);

csif_buf* csif_buf_read_file(char* file_path);
int csif_buf_write_file(char* out_path, csif_buf* out_buff, int clear_buff);
int csif_buf_append_file(char* out_path, csif_buf* out_buff, int clear_buff);

#ifdef __cplusplus
}
#endif

#endif
