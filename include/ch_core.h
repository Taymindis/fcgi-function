#ifndef __ch_h__
#define __ch_h__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifdef __cplusplus
extern "C" {
#endif

#ifdef FDYMEMDETECT
#include <ngxch/fdy_mem_detect.h>
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

#include "ch_pool.h"
#include "ch_hash.h"
#include "ch_map.h"
#include "ch_buf.h"


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
	ch_pool *pool;

	// unsigned long int _identifier;

	// union {
	char *query_str;
	// };
	// char *handler;
	int session_id;
	// char** query_params;
	jmp_buf jump_err_buff;
	sigset_t *curr_mask;
} ch_session_t;

int no_debug(FILE *__restrict __stream, const char *__restrict __format, ...);
int (*ch_dbg)(FILE *__restrict __stream, const char *__restrict __format, ...);

static const char AMPERSAND_DELIM = '&';
static const char EQ_DELIM = '=';

int ch_main (int argc, char *argv[], char* all_func[]);
int init_socket(char* sock_port, int backlog, int workers, int forkable, int signalable, int debugmode, char* logfile, char* solib, char** ch_nmap_func);
int file_existed(const char* fname);
int setup_logger(char* out_file_path);
void print_time(char* buff);

#define ch_write_out(...) FCGX_FPrintF(request->out, __VA_ARGS__)

// #define get_json(ch_session_t) ch_session_t->json
#define ch_alloc(ch_session_t, sz) falloc(&ch_session_t->pool, sz)
#define ch_pool_sz(ch_session_t) blk_size(ch_session_t->pool)
//There is not free as csession will free itself

#define STRINGIFY(x) #x
#define STR(x) STRINGIFY(x)

size_t slen(const char* str);
int ch_isspace(const char* s);

char *duplistr(const char *str);
int is_empty(char *s);

ch_session_t *ch_get_session(void);

// char *getBodyContent(FCGX_Request *request);
long ch_readContent(FCGX_Request *request, char** content);
void* ch_getParam(const char *key, char* query_str);


void ch_write_http_status(FCGX_Request *request, uint16_t code);
void ch_write_default_header(FCGX_Request *request);
void ch_write_jsonp_header(FCGX_Request *request);
void ch_write_json_header(FCGX_Request *request);



// Derive from Zed's Awesome Debug Macros Learn C hardway
// #ifdef _FDEBUG_
// #define fdebug(M, ...) fprintf(FLOGGER_, "DEBUG - %s - %s:%d: " M "\n", print_time(), __FILE__, __LINE__, ##__VA_ARGS__)
// #else
// #define fdebug(M, ...)
// #endif
#ifdef CH_MACOSX
#define fdebug(M, ...) { char buff__[20]; print_time(buff__); fprintf(FLOGGER_, "MACOSX -- DEBUG - %s - %s:%d: " M "\n", buff__, __FILE__, __LINE__, ##__VA_ARGS__); }
#else
#define fdebug(M, ...) { char buff__[20]; print_time(buff__); (*ch_dbg)(FLOGGER_, "DEBUG - %s - %s:%d: " M "\n", buff__, __FILE__, __LINE__, ##__VA_ARGS__); }
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
