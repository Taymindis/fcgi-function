#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include "fdy_buf.h"

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

static FBUF_MALLOC fbuf_malloc = malloc;
static FBUF_FREE fbuf_free = free;

void fbuf_set_allocation_fn(FBUF_MALLOC malloc_fun, FBUF_FREE free_fun) {
	fbuf_malloc = malloc_fun;
	fbuf_free = free_fun;
}

fdy_buff *
fbuf_create(void) {
	fdy_buff *fbuf = fbuf_malloc(sizeof(fdy_buff));

	if (fbuf == NULL) {
		return NULL;
	}
	fbuf->data = NULL;
	fbuf->size = 0;
	return fbuf;
}

fdy_buff *
fbuf_init(const char *s) {
	fdy_buff *fbuf = fbuf_create();
	fbuf_add_(fbuf, s);
	return fbuf;
}

int
fbuf_add(fdy_buff *fbuf, void *buf, size_t new_size) {
	void *new_buf;
	/*Swap*/
	new_buf = fbuf_malloc(fbuf->size + new_size + 1);
	if (new_buf  == NULL) {
		return 0;
	}
	if (fbuf->data) {
		memcpy(new_buf, fbuf->data, fbuf->size);
		fbuf_free(fbuf->data); //Not need to free as pool will free later.
	}
	fbuf->data = (char*)new_buf; //after freed, assigned new address of data
	memcpy(fbuf->data + fbuf->size , buf, new_size);
	fbuf->size += new_size;
	// *(fbuf->data + fbuf->size ) = 0;
	/*End Swap*/
	return 1;
}

int
fbuf_add_(fdy_buff *fbuf, const char *s) {
	void *new_buf;
	size_t size = strlen(s);

	/*Swap*/
	new_buf = fbuf_malloc(fbuf->size + size + 1);

	if (new_buf  == NULL) {
		return 0;
	}

	if (fbuf->data) {
		memcpy(new_buf, fbuf->data, fbuf->size);
		fbuf_free(fbuf->data);
	}

	fbuf->data = new_buf;
	/*End Swap*/
	memcpy(fbuf->data + fbuf->size , s, size);

	fbuf->size += size;

	*(fbuf->data + fbuf->size ) = 0;
	// fbuf_free((char*)s);
	return 1;
}

int fbuf_delete(fdy_buff *fbuf) {
	// Can safely assume fbuf is NULL or fully built.
	if (fbuf != NULL) {
		fbuf_free(fbuf->data);
		fbuf->data = NULL;
		fbuf->size = 0;
		fbuf_free(fbuf);
	}
	return 1;
}

fdy_buff* fbuf_read_file(char* file_path) {
	fdy_buff *fbuf = fbuf_create();

	FILE *f; long len; char *data;
	// cJSON *json_ptr;
	f = fopen(file_path, "rb");
	fseek(f, 0, SEEK_END);
	len = ftell(f);
	fseek(f, 0, SEEK_SET);
	data = (char*)malloc(len + 1);
	fread(data, 1, len, f); data[len] = '\0'; fclose(f);

	fbuf_add(fbuf, data, len);
	fbuf_free(data);
	fclose(f);

	return fbuf;
}

int fbuf_write_file(char* out_path, fdy_buff* out_buff, int clear_buff) {
	FILE* writeFile;

	if ((writeFile = fopen(out_path, "w")) == NULL)
	{	// Open source file.
		perror("fopen source-file\n");
		return 0;
	}

	fwrite(out_buff->data, 1, out_buff->size, writeFile);

	fclose(writeFile);

	if (clear_buff) {
		fbuf_delete(out_buff);
	}

	return 1;
}


int fbuf_append_file(char* out_path, fdy_buff* out_buff, int clear_buff) {
	FILE* writeFile;

	if ((writeFile = fopen(out_path, "a")) == NULL)
	{	// Open source file.
		perror("fopen source-file\n");
		return 0;
	}

	fwrite(out_buff->data, 1, out_buff->size, writeFile);

	fclose(writeFile);

	if (clear_buff) {
		fbuf_delete(out_buff);
	}
	return 1;
}