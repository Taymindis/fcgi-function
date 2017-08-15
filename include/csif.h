#ifndef __csif_h__
#define __csif_h__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#ifdef __cplusplus
extern "C" {
#endif

#ifdef FDYMEMDETECT
#include <csif/fdy_mem_detect.h>
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

// Build in cJSON plugin
#include "csif_json.h"
#include "csif_pool.h"
#include "csif_hash.h"
#include "csif_map.h"
#include "csif_buf.h"
#include "csif_LFHashTable.h"
#include "csif_LFMap.h"

#define FLOGGER_ stderr

#define SPACE (0x20)
#define TAB  (0x09)
#define NEW_LINE  (0x0a)
#define VERTICAL_TAB  (0x0b)
#define FF_FEED  (0x0c)
#define CARRIAGE_RETURN  (0x0d)



typedef struct {
	csif_pool *pool;

	// unsigned long int _identifier;

	// union {
	// cJSON *json;
	char *query_str;
	// };
	// char *handler;
	int session_id;
	// char** query_params;
	jmp_buf jump_err_buff;
	sigset_t *curr_mask;
} csif_t;

int no_debug(FILE *__restrict __stream, const char *__restrict __format, ...);
int (*csif_dbg)(FILE *__restrict __stream, const char *__restrict __format, ...);

static const char AMPERSAND_DELIM = '&';
static const char EQ_DELIM = '=';

int csif_main (int argc, char *argv[], char* all_func[]);
int init_socket(char* sock_port, int backlog, int workers, int forkable, int signalable, int debugmode, char* logfile, char* solib, char** csif_nmap_func);
int file_existed(const char* fname);
int setup_logger(char* out_file_path);
char* print_time(void);

#define csif_write_out(...) FCGX_FPrintF(request->out, __VA_ARGS__)

// #define get_json(csif_t) csif_t->json
#define csif_alloc(csif_t, sz) falloc(&csif_t->pool, sz)
#define csif_pool_sz(csif_t) blk_size(csif_t->pool)
//There is not free as csif will free itself

#define STRINGIFY(x) #x
#define STR(x) STRINGIFY(x)

size_t slen(const char* str);
int csif_isspace(const char* s);

char *duplistr(const char *str);
int is_empty(char *s);

csif_t *csif_get_t(void);

// char *getBodyContent(FCGX_Request *request);
long csif_readContent(FCGX_Request *request, char** content);
void* csif_getParam(const char *key, char* query_str);


void csif_write_http_status(FCGX_Request *request, uint16_t code);
void csif_write_default_header(FCGX_Request *request);
void csif_write_jsonp_header(FCGX_Request *request);
void csif_write_json_header(FCGX_Request *request);



// Derive from Zed's Awesome Debug Macros Learn C hardway
// #ifdef _FDEBUG_
// #define fdebug(M, ...) fprintf(FLOGGER_, "DEBUG - %s - %s:%d: " M "\n", print_time(), __FILE__, __LINE__, ##__VA_ARGS__)
// #else
// #define fdebug(M, ...)
// #endif
#ifdef CSIF_MACOSX
#define fdebug(M, ...) fprintf(FLOGGER_, "MACOSX -- DEBUG - %s - %s:%d: " M "\n", print_time(), __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define fdebug(M, ...) (*csif_dbg)(FLOGGER_, "DEBUG - %s - %s:%d: " M "\n", print_time(), __FILE__, __LINE__, ##__VA_ARGS__)
#endif
#define clean_errno() (errno == 0 ? "None" : strerror(errno))

#define flog_err(M, ...) fprintf(FLOGGER_, "[ERROR] (%s - %s:%d: errno: %s) " M "\n", print_time(), __FILE__, __LINE__, clean_errno(),  ##__VA_ARGS__)

#define flog_warn(M, ...) fprintf(FLOGGER_, "[WARN] (%s - %s:%d: errno: %s) " M "\n", print_time(), __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)

#define flog_info(M, ...) fprintf(FLOGGER_, "[INFO] (%s - %s:%d) " M "\n", print_time(), __FILE__, __LINE__, ##__VA_ARGS__)

#define check(A, M, ...) if(!(A)) { flog_err(M, ##__VA_ARGS__); errno=0; goto error; }

#define sentinel(M, ...)  { flog_err(M, ##__VA_ARGS__); errno=0; goto error; }

#define check_mem(A) check((A), "Out of memory.")

#define check_debug(A, M, ...) if(!(A)) { fdebug(M, ##__VA_ARGS__); errno=0; goto error; }

#ifdef __cplusplus
}
#endif

#endif
