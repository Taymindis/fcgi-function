#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include "ffunc_buf.h"

#include <sys/stat.h>
#include <fcntl.h>

static CH_BUF_MALLOC_FN ffunc_buf_malloc_fn = malloc;
static CH_BUF_FREE_FN ffunc_buf_free_fn = free;

void ffunc_buf_set_alloc_fn(CH_BUF_MALLOC_FN malloc_fun, CH_BUF_FREE_FN free_fun) {
	ffunc_buf_malloc_fn = malloc_fun;
	ffunc_buf_free_fn = free_fun;
}

ffunc_buf *
ffunc_buf_create(void) {
	ffunc_buf *buff_ = ffunc_buf_malloc_fn(sizeof(ffunc_buf));

	if (buff_ == NULL) {
		return NULL;
	}
	buff_->data = NULL;
	buff_->size = 0;
	return buff_;
}

ffunc_buf *
ffunc_buf_init(const char *s) {
	ffunc_buf *buff_ = ffunc_buf_create();
	ffunc_buf_add_(buff_, s);
	return buff_;
}

int
ffunc_buf_add(ffunc_buf *buff_, void *buf, size_t new_size) {
	void *new_buf;
	/*Swap*/
	new_buf = ffunc_buf_malloc_fn(buff_->size + new_size + 1);
	if (new_buf  == NULL) {
		return 0;
	}
	if (buff_->data) {
		memcpy(new_buf, buff_->data, buff_->size);
		ffunc_buf_free_fn(buff_->data); //Not need to free as pool will free later.
	}
	buff_->data = (unsigned char*)new_buf; //after freed, assigned new address of data
	memcpy(buff_->data + buff_->size , buf, new_size);
	buff_->size += new_size;
	// *(buff_->data + buff_->size ) = 0;
	/*End Swap*/
	return 1;
}

int
ffunc_buf_add_(ffunc_buf *buff_, const char *s) {
	void *new_buf;
	size_t size = strlen(s);

	/*Swap*/
	new_buf = ffunc_buf_malloc_fn(buff_->size + size + 1);

	if (new_buf  == NULL) {
		return 0;
	}

	if (buff_->data) {
		memcpy(new_buf, buff_->data, buff_->size);
		ffunc_buf_free_fn(buff_->data);
	}

	buff_->data = new_buf;
	/*End Swap*/
	memcpy(buff_->data + buff_->size , s, size);

	buff_->size += size;

	*(buff_->data + buff_->size ) = 0;
	// fbuf_free((unsigned char*)s);
	return 1;
}

int ffunc_buf_delete(ffunc_buf *buff_) {
	// Can safely assume buff_ is NULL or fully built.
	if (buff_ != NULL) {
		ffunc_buf_free_fn(buff_->data);
		buff_->data = NULL;
		buff_->size = 0;
		ffunc_buf_free_fn(buff_);
	}
	return 1;
}
#if defined __GNUC__ || defined __CYGWIN__ || defined __MINGW32__ || defined __APPLE__
ffunc_buf* ffunc_buf_read_file(char* file_path) {
	ffunc_buf *buff_ = ffunc_buf_create();

	FILE *f; long len; unsigned char *data;

	f = fopen(file_path, "rb");
	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fseek(f, 0, SEEK_SET);
	data = (unsigned char*)malloc(len + 1);
	fread(data, 1, len, f); data[len] = '\0'; fclose(f);

	ffunc_buf_add(buff_, data, len);
	ffunc_buf_free_fn(data);
	fclose(f);

	return buff_;
}

int ffunc_buf_write_file(char* out_path, ffunc_buf* out_buff, int clear_buff) {
	FILE* writeFile;

	if ((writeFile = fopen(out_path, "w")) == NULL)
	{	// Open source file.
		perror("fopen source-file\n");
		return 0;
	}

	fwrite(out_buff->data, 1, out_buff->size, writeFile);

	fclose(writeFile);

	if (clear_buff) {
		ffunc_buf_delete(out_buff);
	}

	return 1;
}


int ffunc_buf_append_file(char* out_path, ffunc_buf* out_buff, int clear_buff) {
	FILE* writeFile;

	if ((writeFile = fopen(out_path, "a")) == NULL)
	{	// Open source file.
		perror("fopen source-file\n");
		return 0;
	}

	fwrite(out_buff->data, 1, out_buff->size, writeFile);

	fclose(writeFile);

	if (clear_buff) {
		ffunc_buf_delete(out_buff);
	}
	return 1;
}
#endif

