#ifndef __ffunc_h__
#define __ffunc_h__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifdef __cplusplus
extern "C" {
#endif

#if defined __GNUC__ || defined __CYGWIN__ || defined __MINGW32__ || defined __APPLE__
#include <stdio.h>
#include <stdlib.h>
#include <fcgiapp.h>
#include <errno.h>
#include <string.h>
#include <dlfcn.h>
#include <time.h>
#include <locale.h>
#include <sys/time.h>
#define FFUNC void
#else
#define FFUNC extern __declspec(dllexport) void
#include <stdio.h>
#include <Windows.h>
#include <fcgiapp.h>
#endif

#ifndef __APPLE__	
#include <malloc.h>
#endif
#define DEFAULT_BLK_SZ 2048
#define ALIGNMENT   16

typedef struct pool {
	void *next,
	     *end;
} ffunc_pool;

typedef struct {
	ffunc_pool *pool;
	char *query_str;
	FCGX_Request *request;
} ffunc_session_t;

typedef struct {
	char *data;
	size_t len;
} ffunc_str_t;

extern ffunc_pool* ffunc_create_pool( size_t size );
extern void ffunc_destroy_pool( ffunc_pool *p );
extern void * _ffunc_alloc( ffunc_pool **p, size_t size );
extern size_t ffunc_mem_left( ffunc_pool *p );
extern size_t ffunc_blk_size( ffunc_pool *p );

#define FLOGGER_ stderr

#define SPACE (0x20)
#define TAB  (0x09)
#define NEW_LINE  (0x0a)
#define VERTICAL_TAB  (0x0b)
#define FF_FEED  (0x0c)
#define CARRIAGE_RETURN  (0x0d)

static const char AMPERSAND_DELIM = '&';
static const char EQ_DELIM = '=';

extern int ffunc_main(int sock_port, int backlog, int max_thread, char** ffunc_nmap_func, void (*app_init_handler)(void));
extern int ffunc_main2(int sock_port, int backlog, int max_thread, char** ffunc_nmap_func, void (*app_init_handler)(void), size_t max_read_buffer);
extern int ffunc_file_existd(const char* fname);
extern void ffunc_print_time(char* buff);

#define ffunc_write_out(_csession, ...) FCGX_FPrintF(_csession->request->out, __VA_ARGS__)
#define ffunc_get_fcgi_param(_csession, constkey) FCGX_GetParam(constkey, _csession->request->envp)
#define ffunc_alloc(ffunc_session_t, sz) _ffunc_alloc(&ffunc_session_t->pool, sz)
#define ffunc_pool_sz(ffunc_session_t) blk_size(ffunc_session_t->pool)
#define STRINGIFY(x) #x
#define STR(x) STRINGIFY(x)

extern size_t slen(const char* str);
extern int ffunc_isspace(const char* s);

extern char *duplistr(ffunc_session_t * csession, const char *str);
extern int is_empty(char *s);

extern size_t (*ffunc_read_body)(ffunc_session_t * , ffunc_str_t *);
extern void* ffunc_get_query_param(ffunc_session_t * csession, const char *key, size_t len);
extern void ffunc_write_http_ok_status(ffunc_session_t * csession);
extern void ffunc_write_http_not_found_status(ffunc_session_t * csession);
extern void ffunc_write_http_internal_error_status(ffunc_session_t * csession);
extern void ffunc_write_http_no_content_status(ffunc_session_t * csession);
extern void ffunc_write_default_header(ffunc_session_t * csession);
extern void ffunc_write_jsonp_header(ffunc_session_t * csession);
extern void ffunc_write_json_header(ffunc_session_t * csession);




#ifdef __cplusplus
}
#endif

#endif
