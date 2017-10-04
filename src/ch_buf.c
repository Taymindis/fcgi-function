#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include "ch_buf.h"

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

static CH_BUF_MALLOC_FN ch_buf_malloc_fn = malloc;
static CH_BUF_FREE_FN ch_buf_free_fn = free;

void ch_buf_set_alloc_fn(CH_BUF_MALLOC_FN malloc_fun, CH_BUF_FREE_FN free_fun) {
	ch_buf_malloc_fn = malloc_fun;
	ch_buf_free_fn = free_fun;
}

ch_buf *
ch_buf_create(void) {
	ch_buf *buff_ = ch_buf_malloc_fn(sizeof(ch_buf));

	if (buff_ == NULL) {
		return NULL;
	}
	buff_->data = NULL;
	buff_->size = 0;
	return buff_;
}

ch_buf *
ch_buf_init(const char *s) {
	ch_buf *buff_ = ch_buf_create();
	ch_buf_add_(buff_, s);
	return buff_;
}

int
ch_buf_add(ch_buf *buff_, void *buf, size_t new_size) {
	void *new_buf;
	/*Swap*/
	new_buf = ch_buf_malloc_fn(buff_->size + new_size + 1);
	if (new_buf  == NULL) {
		return 0;
	}
	if (buff_->data) {
		memcpy(new_buf, buff_->data, buff_->size);
		ch_buf_free_fn(buff_->data); //Not need to free as pool will free later.
	}
	buff_->data = (unsigned char*)new_buf; //after freed, assigned new address of data
	memcpy(buff_->data + buff_->size , buf, new_size);
	buff_->size += new_size;
	// *(buff_->data + buff_->size ) = 0;
	/*End Swap*/
	return 1;
}

int
ch_buf_add_(ch_buf *buff_, const char *s) {
	void *new_buf;
	size_t size = strlen(s);

	/*Swap*/
	new_buf = ch_buf_malloc_fn(buff_->size + size + 1);

	if (new_buf  == NULL) {
		return 0;
	}

	if (buff_->data) {
		memcpy(new_buf, buff_->data, buff_->size);
		ch_buf_free_fn(buff_->data);
	}

	buff_->data = new_buf;
	/*End Swap*/
	memcpy(buff_->data + buff_->size , s, size);

	buff_->size += size;

	*(buff_->data + buff_->size ) = 0;
	// fbuf_free((unsigned char*)s);
	return 1;
}

int ch_buf_delete(ch_buf *buff_) {
	// Can safely assume buff_ is NULL or fully built.
	if (buff_ != NULL) {
		ch_buf_free_fn(buff_->data);
		buff_->data = NULL;
		buff_->size = 0;
		ch_buf_free_fn(buff_);
	}
	return 1;
}

ch_buf* ch_buf_read_file(char* file_path) {
	ch_buf *buff_ = ch_buf_create();

	FILE *f; long len; unsigned char *data;

	f = fopen(file_path, "rb");
	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fseek(f, 0, SEEK_SET);
	data = (unsigned char*)malloc(len + 1);
	fread(data, 1, len, f); data[len] = '\0'; fclose(f);

	ch_buf_add(buff_, data, len);
	ch_buf_free_fn(data);
	fclose(f);

	return buff_;
}

int ch_buf_write_file(char* out_path, ch_buf* out_buff, int clear_buff) {
	FILE* writeFile;

	if ((writeFile = fopen(out_path, "w")) == NULL)
	{	// Open source file.
		perror("fopen source-file\n");
		return 0;
	}

	fwrite(out_buff->data, 1, out_buff->size, writeFile);

	fclose(writeFile);

	if (clear_buff) {
		ch_buf_delete(out_buff);
	}

	return 1;
}


int ch_buf_append_file(char* out_path, ch_buf* out_buff, int clear_buff) {
	FILE* writeFile;

	if ((writeFile = fopen(out_path, "a")) == NULL)
	{	// Open source file.
		perror("fopen source-file\n");
		return 0;
	}

	fwrite(out_buff->data, 1, out_buff->size, writeFile);

	fclose(writeFile);

	if (clear_buff) {
		ch_buf_delete(out_buff);
	}
	return 1;
}
