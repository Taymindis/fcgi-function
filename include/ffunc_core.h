#ifndef __ffunc_h__
#define __ffunc_h__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifdef __cplusplus
extern "C" {
#endif

#ifdef FDYMEMDETECT
#include <ffunc/fdy_mem_detect.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcgiapp.h>
#include <errno.h>
#include <string.h>
#include <dlfcn.h>
#include <time.h>
#include <locale.h>
#include <sys/time.h>

#include "ffunc_pool.h"
#include "ffunc_buf.h"

#define FLOGGER_ stderr

#define SPACE (0x20)
#define TAB  (0x09)
#define NEW_LINE  (0x0a)
#define VERTICAL_TAB  (0x0b)
#define FF_FEED  (0x0c)
#define CARRIAGE_RETURN  (0x0d)

typedef struct {
	ffunc_pool *pool;
	char *query_str;
	FCGX_Request *request;
	// sigset_t *curr_mask;
} ffunc_session_t;

typedef struct {
	char *data;
	size_t len;
} ffunc_str_t;

static const char AMPERSAND_DELIM = '&';
static const char EQ_DELIM = '=';

extern int ffunc_main(int sock_port, int backlog, int max_thread, char** ffunc_nmap_func, void (*app_init_handler)(void));
extern int ffunc_main2(int sock_port, int backlog, int max_thread, char** ffunc_nmap_func, void (*app_init_handler)(void), size_t max_read_buffer);
extern int ffunc_file_existd(const char* fname);
extern void ffunc_print_time(char* buff);

#define ffunc_write_out(_csession, ...) FCGX_FPrintF(_csession->request->out, __VA_ARGS__)
#define ffunc_alloc(ffunc_session_t, sz) falloc(&ffunc_session_t->pool, sz)
#define ffunc_pool_sz(ffunc_session_t) blk_size(ffunc_session_t->pool)
#define STRINGIFY(x) #x
#define STR(x) STRINGIFY(x)

size_t slen(const char* str);
int ffunc_isspace(const char* s);

char *duplistr(ffunc_session_t * csession, const char *str);
int is_empty(char *s);


// char *getBodyContent(FCGX_Request *request);
extern size_t (*ffunc_read_body)(ffunc_session_t * , ffunc_str_t *);
void* ffunc_get_query_param(ffunc_session_t * csession, const char *key, size_t len);

void ffunc_write_http_status(ffunc_session_t * csession, uint16_t code);
void ffunc_write_default_header(ffunc_session_t * csession);
void ffunc_write_jsonp_header(ffunc_session_t * csession);
void ffunc_write_json_header(ffunc_session_t * csession);


#ifdef __cplusplus
}
#endif

#endif
