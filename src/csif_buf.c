#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include "csif_buf.h"

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

static CSIF_BUF_MALLOC_FN csif_buf_malloc_fn = malloc;
static CSIF_BUF_FREE_FN csif_buf_free_fn = free;

void csif_buf_set_alloc_fn(CSIF_BUF_MALLOC_FN malloc_fun, CSIF_BUF_FREE_FN free_fun) {
	csif_buf_malloc_fn = malloc_fun;
	csif_buf_free_fn = free_fun;
}

csif_buf *
csif_buf_create(void) {
	csif_buf *buff_ = csif_buf_malloc_fn(sizeof(csif_buf));

	if (buff_ == NULL) {
		return NULL;
	}
	buff_->data = NULL;
	buff_->size = 0;
	return buff_;
}

csif_buf *
csif_buf_init(const char *s) {
	csif_buf *buff_ = csif_buf_create();
	csif_buf_add_(buff_, s);
	return buff_;
}

int
csif_buf_add(csif_buf *buff_, void *buf, size_t new_size) {
	void *new_buf;
	/*Swap*/
	new_buf = csif_buf_malloc_fn(buff_->size + new_size + 1);
	if (new_buf  == NULL) {
		return 0;
	}
	if (buff_->data) {
		memcpy(new_buf, buff_->data, buff_->size);
		csif_buf_free_fn(buff_->data); //Not need to free as pool will free later.
	}
	buff_->data = (char*)new_buf; //after freed, assigned new address of data
	memcpy(buff_->data + buff_->size , buf, new_size);
	buff_->size += new_size;
	// *(buff_->data + buff_->size ) = 0;
	/*End Swap*/
	return 1;
}

int
csif_buf_add_(csif_buf *buff_, const char *s) {
	void *new_buf;
	size_t size = strlen(s);

	/*Swap*/
	new_buf = csif_buf_malloc_fn(buff_->size + size + 1);

	if (new_buf  == NULL) {
		return 0;
	}

	if (buff_->data) {
		memcpy(new_buf, buff_->data, buff_->size);
		csif_buf_free_fn(buff_->data);
	}

	buff_->data = new_buf;
	/*End Swap*/
	memcpy(buff_->data + buff_->size , s, size);

	buff_->size += size;

	*(buff_->data + buff_->size ) = 0;
	// fbuf_free((char*)s);
	return 1;
}

int csif_buf_delete(csif_buf *buff_) {
	// Can safely assume buff_ is NULL or fully built.
	if (buff_ != NULL) {
		csif_buf_free_fn(buff_->data);
		buff_->data = NULL;
		buff_->size = 0;
		csif_buf_free_fn(buff_);
	}
	return 1;
}

csif_buf* csif_buf_read_file(char* file_path) {
	csif_buf *buff_ = csif_buf_create();

	FILE *f; long len; char *data;
	// cJSON *json_ptr;
	f = fopen(file_path, "rb");
	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fseek(f, 0, SEEK_SET);
	data = (char*)malloc(len + 1);
	fread(data, 1, len, f); data[len] = '\0'; fclose(f);

	csif_buf_add(buff_, data, len);
	csif_buf_free_fn(data);
	fclose(f);

	return buff_;
}

int csif_buf_write_file(char* out_path, csif_buf* out_buff, int clear_buff) {
	FILE* writeFile;

	if ((writeFile = fopen(out_path, "w")) == NULL)
	{	// Open source file.
		perror("fopen source-file\n");
		return 0;
	}

	fwrite(out_buff->data, 1, out_buff->size, writeFile);

	fclose(writeFile);

	if (clear_buff) {
		csif_buf_delete(out_buff);
	}

	return 1;
}


int csif_buf_append_file(char* out_path, csif_buf* out_buff, int clear_buff) {
	FILE* writeFile;

	if ((writeFile = fopen(out_path, "a")) == NULL)
	{	// Open source file.
		perror("fopen source-file\n");
		return 0;
	}

	fwrite(out_buff->data, 1, out_buff->size, writeFile);

	fclose(writeFile);

	if (clear_buff) {
		csif_buf_delete(out_buff);
	}
	return 1;
}
