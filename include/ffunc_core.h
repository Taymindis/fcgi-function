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
#include <setjmp.h>
#include <sys/time.h>

#include "ffunc_pool.h"
#include "ffunc_hash.h"
#include "ffunc_map.h"
#include "ffunc_buf.h"


#include "binary_array.h"
#include "atomic_hashtable.h"
#include "atomic_hashtable_n.h"	

#define FLOGGER_ stderr

#define SPACE (0x20)
#define TAB  (0x09)
#define NEW_LINE  (0x0a)
#define VERTICAL_TAB  (0x0b)
#define FF_FEED  (0x0c)
#define CARRIAGE_RETURN  (0x0d)



typedef struct {
	ffunc_pool *pool;

	// unsigned long int _identifier;

	// union {
	char *query_str;
	// };
	// char *handler;
	int session_id;
	// char** query_params;
	jmp_buf jump_err_buff;
	sigset_t *curr_mask;
} ffunc_session_t;

int no_debug(FILE *__restrict __stream, const char *__restrict __format, ...);
int (*ffunc_dbg)(FILE *__restrict __stream, const char *__restrict __format, ...);

static const char AMPERSAND_DELIM = '&';
static const char EQ_DELIM = '=';

int ffunc_main (int argc, char *argv[], char* all_func[]);
int init_socket(char* sock_port, int backlog, int workers, int forkable, int signalable, int debugmode, char* logfile, char* solib, char** ffunc_nmap_func);
int file_existed(const char* fname);
int setup_logger(char* out_file_path);
void print_time(char* buff);

#define ffunc_write_out(...) FCGX_FPrintF(request->out, __VA_ARGS__)

// #define get_json(ffunc_session_t) ffunc_session_t->json
#define ffunc_alloc(ffunc_session_t, sz) falloc(&ffunc_session_t->pool, sz)
#define ffunc_pool_sz(ffunc_session_t) blk_size(ffunc_session_t->pool)
//There is not free as csession will free itself

#define STRINGIFY(x) #x
#define STR(x) STRINGIFY(x)

size_t slen(const char* str);
int ffunc_isspace(const char* s);

char *duplistr(const char *str);
int is_empty(char *s);

ffunc_session_t *ffunc_get_session(void);

// char *getBodyContent(FCGX_Request *request);
long ffunc_readContent(FCGX_Request *request, char** content);
void* ffunc_getParam(const char *key, char* query_str);


void ffunc_write_http_status(FCGX_Request *request, uint16_t code);
void ffunc_write_default_header(FCGX_Request *request);
void ffunc_write_jsonp_header(FCGX_Request *request);
void ffunc_write_json_header(FCGX_Request *request);



// Derive from Zed's Awesome Debug Macros Learn C hardway
// #ifdef _FDEBUG_
// #define fdebug(M, ...) fprintf(FLOGGER_, "DEBUG - %s - %s:%d: " M "\n", print_time(), __FILE__, __LINE__, ##__VA_ARGS__)
// #else
// #define fdebug(M, ...)
// #endif
#ifdef CH_MACOSX
#define fdebug(M, ...) { char buff__[20]; print_time(buff__); fprintf(FLOGGER_, "MACOSX -- DEBUG - %s - %s:%d: " M "\n", buff__, __FILE__, __LINE__, ##__VA_ARGS__); }
#else
#define fdebug(M, ...) { char buff__[20]; print_time(buff__); (*ffunc_dbg)(FLOGGER_, "DEBUG - %s - %s:%d: " M "\n", buff__, __FILE__, __LINE__, ##__VA_ARGS__); }
#endif
#define clean_errno() (errno == 0 ? "None" : strerror(errno))

#define flog_err(M, ...) { char buff__[20]; print_time(buff__); fprintf(FLOGGER_, "[ERROR] (%s - %s:%d: errno: %s) " M "\n", buff__, __FILE__, __LINE__, clean_errno(),  ##__VA_ARGS__); }

#define flog_warn(M, ...) { char buff__[20]; print_time(buff__);  fprintf(FLOGGER_, "[WARN] (%s - %s:%d: errno: %s) " M "\n", buff__, __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__); }

#define flog_info(M, ...) { char buff__[20]; print_time(buff__);  fprintf(FLOGGER_, "[INFO] (%s - %s:%d) " M "\n", buff__, __FILE__, __LINE__, ##__VA_ARGS__); }

#define check(A, M, ...) if(!(A)) { flog_err(M, ##__VA_ARGS__); errno=0; goto error; }

#define sentinel(M, ...)  { flog_err(M, ##__VA_ARGS__); errno=0; goto error; }

#define check_mem(A) check((A), "Out of memory.")

#define check_debug(A, M, ...) if(!(A)) { fdebug(M, ##__VA_ARGS__); errno=0; goto error; }

#ifdef __cplusplus
}
#endif

#endif
