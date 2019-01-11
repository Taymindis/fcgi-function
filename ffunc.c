#if defined __GNUC__ || defined __CYGWIN__ || defined __MINGW32__ || defined __APPLE__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <execinfo.h>
#include <unistd.h> /* needed to define getpid() */
#include <signal.h>
// #include <setjmp.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcgi_config.h>
#include <fcgiapp.h>
#include <dlfcn.h>
#include <pthread.h>
#include <sys/socket.h>
#include "ffunc.h"

#define _get_param_(KEY) FCGX_GetParam(KEY, request->envp)
#define ffunc_print(...) fprintf (stderr, __VA_ARGS__)

static void* mem_align(size_t size);
static ffunc_pool* ffunc_recreate_pool(ffunc_pool *curr_p, size_t new_size);

typedef void (*h)(ffunc_session_t*);

struct ffunc_handler {
    char* name;
    size_t len;
    h handle;
};

typedef struct  {
    int fcgi_func_socket;
    pthread_mutex_t accept_mutex;
} ffunc_worker_t;

static int func_count = 0;
struct ffunc_handler *dyna_funcs = NULL;

static size_t max_std_input_buffer;

static int ffunc_init(char** ffunc_nmap_func);
int strpos(const char *haystack, const char *needle);
void *ffunc_thread_worker(void* wrker);
static int hook_socket(int sock_port, int backlog, int max_thread, char** ffunc_nmap_func);
static void ffunc_add_signal_handler(void);
static void handle_request(FCGX_Request *request);
static size_t ffunc_read_body_limit(ffunc_session_t * csession, ffunc_str_t *content);
static size_t ffunc_read_body_nolimit(ffunc_session_t * csession, ffunc_str_t *content);
static int  has_init_signal = 0;
static struct sigaction sa;
static void *usr_req_handle;
// extern declaration
size_t (*ffunc_read_body)(ffunc_session_t * , ffunc_str_t *);

static int
ffunc_init(char** ffunc_nmap_func) {
    int f = 0;
    size_t szof_func_name;
    usr_req_handle = NULL;
    char *error;
    usr_req_handle = dlopen(NULL, RTLD_LAZY | RTLD_NOW);
    if (!usr_req_handle) {
        ffunc_print("%s\n", dlerror());
        ffunc_print("%s\n", "Have you included -rdynamic in your compiler option, for e.g gcc ... -rdynamic");
        return 0;
    }

    for (func_count = 0; ffunc_nmap_func[func_count]; func_count++);

    dyna_funcs = (struct ffunc_handler*) malloc(func_count * sizeof(struct ffunc_handler));

    for (f = 0; f < func_count; f++) {
        szof_func_name = strlen(ffunc_nmap_func[f]);
        dyna_funcs[f].name = (char*) calloc(szof_func_name + 1, sizeof(char));

        memcpy(dyna_funcs[f].name, ffunc_nmap_func[f], szof_func_name);
        dyna_funcs[f].len = szof_func_name;

        *(void **) (&dyna_funcs[f].handle) = dlsym(usr_req_handle, ffunc_nmap_func[f]);

        if ((error = dlerror()) != NULL) {
            ffunc_print("%s\n", error);
            ffunc_print("%s\n", "Have you included -rdynamic in your compiler option, for e.g gcc ... -rdynamic");
            return 0;
        }
    }
    return 1;
}

/**To prevent -std=99 issue**/
char *
ffunc_strdup(ffunc_session_t * csession, const char *str, size_t len) {
    if (len == 0 ) {
        return NULL;
    }
    char *dupstr = (char*)_ffunc_alloc(&csession->pool, len + 1);
    if (dupstr == NULL) {
        return NULL;
    }
    memset(dupstr, 0, len + 1);
    return memcpy(dupstr, str, len);
}

void
ffunc_print_time(char* buff) {
    time_t now = time(NULL);
    strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));
}

static void
handle_request(FCGX_Request *request) {
    char *value,
         *qparam,
         *query_string;

    ffunc_pool *p = ffunc_create_pool(DEFAULT_BLK_SZ);
    ffunc_session_t *_csession_ = (ffunc_session_t*)malloc(sizeof(ffunc_session_t));
    if ( p == NULL || _csession_ == NULL) {
        fprintf(stderr, "error %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    _csession_->pool = p;
    _csession_->query_str = NULL;
    _csession_->request = request;

    if ( (value = FCGX_GetParam("FN_HANDLER", request->envp)) ) {
        int i;
        size_t vallen = strlen(value);
        for (i = 0; i < func_count; i++) {
            if ( dyna_funcs[i].len == vallen && (strcmp(dyna_funcs[i].name, value) == 0) ) {
                if ( ( qparam = _get_param_("QUERY_STRING") ) ) {
                    query_string = ffunc_strdup(_csession_, qparam, strlen(qparam));
                    if (!is_empty(query_string)) {
                        _csession_->query_str = query_string;//parse_json_fr_args(query_string);
                    }
                }
                dyna_funcs[i].handle(_csession_);

                goto RELEASE_POOL;
            }
        }
    } else {
        ffunc_write_out(_csession_, "Content-Type: text/plain\r\n\r\n");
        ffunc_write_out(_csession_, "%s\n", "FN_HANDLER not found");
        ffunc_write_out(_csession_, "%s\n", "Please provide your FN_HANDLER in your config file");
        ffunc_write_out(_csession_, "%s\n", "For e.g nginx.conf fastcgi_param FN_HANDLER getProfile");
    }


RELEASE_POOL:
    ffunc_destroy_pool(_csession_->pool);
    free(_csession_);
}

size_t
slen(const char* str) {
    register const char *s;
    for (s = str; *s; ++s);
    return (s - str);
}

int
ffunc_isspace(const char* s) {
    return (*s == SPACE || *s == TAB || *s == NEW_LINE ||
            *s == VERTICAL_TAB || *s == FF_FEED || *s == CARRIAGE_RETURN ? 1 : 0);
    // return (*s == '\t' || *s == '\n' ||
    //         *s == '\v' || *s == '\f' || *s == '\r' || *s == ' ' ? 1 : 0);
}

int
is_empty(char *s) {
    if (s && s[0])
        return 0;
    return 1;
}

int
ffunc_file_existd(const char* fname) {
    if ( access( fname, F_OK ) != -1 )
        return 1;
    else
        return 0;
}

int
strpos(const char *haystack, const char *needle)
{
    char *p = (char*)strstr(haystack, needle);
    if (p)
        return p - haystack;
    return -1;   // Not found = -1.
}

void*
ffunc_get_query_param(ffunc_session_t * csession, const char *key, size_t len) {
    int pos;
    char *qs = csession->query_str;
    if (key && *key && qs && *qs) {
        do {
            if ((pos = strpos(qs, key)) < 0) return NULL;

            if (pos == 0 || qs[pos - 1] == '&') {
                qs = (char*)qs + pos + len;
                if (*qs++ == '=') {
                    char *src = qs,
                          *ret;
                    size_t sz = 0;
                    while (*qs && *qs++ != '&')sz++;

                    ret = (char*) _ffunc_alloc(&csession->pool, sz + 1);
                    memset(ret, 0, sz + 1);
                    return memcpy(ret, src, sz);
                } else while (*qs && *qs++ != '&');
            } else while (*qs && *qs++ != '&');
        } while (*qs);
    }
    return NULL;
}

static size_t
ffunc_read_body_nolimit(ffunc_session_t * csession, ffunc_str_t *content) {
    FCGX_Request *request = csession->request;
    char* clenstr = _get_param_("CONTENT_LENGTH");
    FCGX_Stream *in = request->in;
    size_t len;

    if (clenstr && ((len = strtol(clenstr, &clenstr, 10)) != 0) ) {
        content->data = (char*)_ffunc_alloc(&csession->pool, len + 1);
        memset(content->data, 0, len + 1);
        content->len = FCGX_GetStr(content->data, len, in);
    } else {
        content->data = NULL;
        content->len = 0;
    }

    /* Chew up whats remaining (required by mod_fastcgi) */
    while (FCGX_GetChar(in) != -1);

    return len;
}

static size_t
ffunc_read_body_limit(ffunc_session_t * csession, ffunc_str_t *content) {
    FCGX_Request *request = csession->request;
    char* clenstr = _get_param_("CONTENT_LENGTH");
    FCGX_Stream *in = request->in;
    size_t len;

    if (clenstr) {
        // long int strtol (const char* str, char** endptr, int base);
        // endptr -- Reference to an object of type char*, whose value is set by the function to the next character in str after the numerical value.
        // This parameter can also be a null pointer, in which case it is not used.
        len = strtol(clenstr, &clenstr, 10);
        /* Don't read more than the predefined limit  */
        if (len > max_std_input_buffer) { len = max_std_input_buffer; }

        content->data = (char*) _ffunc_alloc(&csession->pool, len + 1);
        memset(content->data, 0, len + 1);
        content->len = FCGX_GetStr(content->data, len, in);
    }
    /* Don't read if CONTENT_LENGTH is missing or if it can't be parsed */
    else {
        content->data = NULL;
        content->len = 0;
    }

    /* Chew up whats remaining (required by mod_fastcgi) */
    while (FCGX_GetChar(in) != -1);

    return len;
}


#define FFUNC_APP_INITIALIZING "INIT"
#define FFUNC_APP_INITIALIZED "DONE"
#define FFUNC_APP_INIT_PIPE_BUF_SIZE sizeof(FFUNC_APP_INITIALIZED)

/**Main api**/
int
ffunc_main(int sock_port, int backlog, int max_thread, char** ffunc_nmap_func, void (*app_init_handler)(void)) {
    return ffunc_main2(sock_port, backlog, max_thread, ffunc_nmap_func, app_init_handler, 0);
}
/**Main api**/
int
ffunc_main2(int sock_port, int backlog, int max_thread, char** ffunc_nmap_func, void (*app_init_handler)(void), size_t max_read_buffer) {
    pid_t childPID;
    int child_status;
    int procpip[2];
    char pipbuf[FFUNC_APP_INIT_PIPE_BUF_SIZE];

    if (max_read_buffer > 0) {
        max_std_input_buffer = max_read_buffer;
        ffunc_read_body = &ffunc_read_body_limit;
    } else {
        ffunc_read_body = &ffunc_read_body_nolimit;
    }

    if (!ffunc_nmap_func[0]) {
        ffunc_print("%s\n", "ffunc_nmap_func has no defined...");
        return 1;
    }

    ffunc_print("%s\n", "Service starting");
    ffunc_print("sock_port=%d, backlog=%d\n", sock_port, backlog);
    /** Do master pipe before fork **/
    if (pipe(procpip) < 0)
        exit(1);

FFUNC_WORKER_RESTART:
    if (!has_init_signal) {
        ffunc_add_signal_handler();
    }
    write(procpip[1], FFUNC_APP_INITIALIZING, FFUNC_APP_INIT_PIPE_BUF_SIZE);
    childPID = fork();
    if (childPID >= 0) {// fork was successful
        if (childPID == 0) {// child process
            if (app_init_handler) {
                app_init_handler();
            }
            read(procpip[0], pipbuf, FFUNC_APP_INIT_PIPE_BUF_SIZE);
            write(procpip[1], FFUNC_APP_INITIALIZED, FFUNC_APP_INIT_PIPE_BUF_SIZE);
            close(procpip[0]);
            close(procpip[1]);

            return hook_socket(sock_port, backlog, max_thread, ffunc_nmap_func);
        } else { // Parent process
            while (1) {
                if (waitpid(childPID, &child_status, WNOHANG) == childPID) {
                    has_init_signal = 0;
                    if ( read(procpip[0], pipbuf, FFUNC_APP_INIT_PIPE_BUF_SIZE) > 0 ) {
                        if ( strncmp(pipbuf, FFUNC_APP_INITIALIZED, FFUNC_APP_INIT_PIPE_BUF_SIZE) == 0 ) {
                            goto FFUNC_WORKER_RESTART;
                        } else {
                            printf("%s\n", "Failed while initializing the application... ");
                            close(procpip[0]);
                            close(procpip[1]);
                            return 1; // ERROR
                        }
                    }
                }
                sleep(1);
            }
        }
    } else {// fork failed
        ffunc_print("%s\n", "Fork Child failed..");
        return -1;
    }
    return 0;
    // return hook_socket(sock_port, backlog, max_thread, ffunc_nmap_func, app_init_handler);
}

void *
ffunc_thread_worker(void* wrker) {
    int rc;
    ffunc_worker_t *worker_t = (ffunc_worker_t*) wrker;

    FCGX_Request request;
    if (FCGX_InitRequest(&request, worker_t->fcgi_func_socket, FCGI_FAIL_ACCEPT_ON_INTR) != 0) {
        ffunc_print("%s\n", "Can not init request");
        return NULL;
    }
    while (1) {
        pthread_mutex_lock(&worker_t->accept_mutex);
        rc = FCGX_Accept_r(&request);
        pthread_mutex_unlock(&worker_t->accept_mutex);
        if (rc < 0) {
            ffunc_print("%s\n", "Cannot accept new request");
            FCGX_Finish_r(&request); // this will free all the fcgiparams memory and request
            continue;
        }
        handle_request(&request);
        FCGX_Finish_r(&request); // this will free all the fcgiparams memory and request
    }
    pthread_exit(NULL);
}

static int
hook_socket(int sock_port, int backlog, int max_thread, char** ffunc_nmap_func) {
    FCGX_Init();
    if (!ffunc_init(ffunc_nmap_func)) {
        exit(1);
    }

    char port_str[12];
    ffunc_worker_t *worker_t = (ffunc_worker_t*) malloc(sizeof(ffunc_worker_t));
    if (worker_t == NULL) {
        perror("nomem");
    }

    worker_t->accept_mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER; // this is lock for all threads
    int i;
    sprintf(port_str, ":%d", sock_port);

    if (sock_port) {
        if (backlog)
            worker_t->fcgi_func_socket = FCGX_OpenSocket(port_str, backlog);
        else
            worker_t->fcgi_func_socket = FCGX_OpenSocket(port_str, 50);
    } else {
        ffunc_print("%s\n", "argument wrong");
        exit(1);
    }

    if (worker_t->fcgi_func_socket < 0) {
        ffunc_print("%s\n", "unable to open the socket" );
        exit(1);
    }

    ffunc_print("%d threads \n", max_thread);
    ffunc_print("Socket on hook %d\n", sock_port);

    pthread_t pth_workers[max_thread];
    for ( i = 0; i < max_thread; i++) {
        pthread_create(&pth_workers[i], NULL, ffunc_thread_worker, worker_t);
        // pthread_detach(pth_worker);
    }
    for ( i = 0; i < max_thread; i++) {
        pthread_join(pth_workers[i], NULL);
    }

    ffunc_print("%s\n", "Exiting");
    return EXIT_SUCCESS;
}

static void
ffunc_signal_backtrace(int sfd) {
    size_t i, ptr_size;
    void *buffer[10];
    char **strings;

    ptr_size = backtrace(buffer, 1024);
    fprintf(stderr, "backtrace() returned %zd addresses\n", ptr_size);

    strings = backtrace_symbols(buffer, ptr_size);
    if (strings == NULL) {
        fprintf(stderr, "backtrace_symbols= %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < ptr_size; i++)
        fprintf(stderr, "%s\n", strings[i]);

    free(strings);
    exit(EXIT_FAILURE);
}

static void
ffunc_add_signal_handler() {
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = ffunc_signal_backtrace;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
    sigaction(SIGIOT, &sa, NULL);
    sigaction(SIGSEGV, &sa, NULL);
#ifdef SIGBUS
    sigaction(SIGBUS, &sa, NULL);
#endif
    has_init_signal = 1;
}
#else
#if defined _WIN32 || _WIN64 /*Windows*/
#include <process.h>
#include <stdio.h>
#include <windows.h>
#include <signal.h>
#include <time.h>
#include "ffunc.h"
#include <fcgiapp.h>

#define _get_param_(KEY) FCGX_GetParam(KEY, request->envp)
#define ffunc_print(...) fprintf (stderr, __VA_ARGS__)
#define FFUNC_ERROR(errmsg) fprintf(stderr, "%s - %d\n", errmsg, GetLastError() )


static void* mem_align(size_t size);
static ffunc_pool* ffunc_recreate_pool(ffunc_pool *curr_p, size_t new_size);
typedef void(*h)(ffunc_session_t*);

struct ffunc_handler {
    char* name;
    size_t len;
    h handle;
};

typedef struct {
    int fcgi_func_socket;
    HANDLE accept_mutex;
} ffunc_worker_t;

static int func_count = 0;
struct ffunc_handler *dyna_funcs = NULL;

static size_t max_std_input_buffer;

static int ffunc_init(char** ffunc_nmap_func);
int strpos(const char *haystack, const char *needle);
unsigned __stdcall ffunc_thread_worker(void *wrker);
static int hook_socket(int sock_port, int backlog, int max_thread, char** ffunc_nmap_func);
static void ffunc_add_signal_handler(void);
static void handle_request(FCGX_Request *request);
static size_t ffunc_read_body_limit(ffunc_session_t * csession, ffunc_str_t *content);
static size_t ffunc_read_body_nolimit(ffunc_session_t * csession, ffunc_str_t *content);
static int  has_init_signal = 0;
static HINSTANCE usr_req_handle;


// extern declaration
size_t(*ffunc_read_body)(ffunc_session_t *, ffunc_str_t *);

// static void*
// read_t(void *origin) {
//     ffunc_session_t *_csession_ = malloc(sizeof(ffunc_session_t));
//     memcpy(_csession_, origin, sizeof(ffunc_session_t));
//     return _csession_;
// }

// static void
// free_t(void* node) {
//     free(node);
// }
static int
ffunc_init(char** ffunc_nmap_func) {
    int f;
    SIZE_T szof_func_name;
    usr_req_handle = GetModuleHandle(NULL);
    // Pending FreeLibrary

    if (!usr_req_handle) {
        FFUNC_ERROR("GEGetModuleHandle");
        return 0;
    }

    for (func_count = 0; ffunc_nmap_func[func_count]; func_count++);

    dyna_funcs = malloc(func_count * sizeof(struct ffunc_handler));

    for (f = 0; f < func_count; f++) {
        szof_func_name = strlen(ffunc_nmap_func[f]);
        dyna_funcs[f].name = calloc(szof_func_name + 1, sizeof(char));

        memcpy(dyna_funcs[f].name, ffunc_nmap_func[f], szof_func_name);
        dyna_funcs[f].len = szof_func_name;

        dyna_funcs[f].handle = (h) GetProcAddress(usr_req_handle, ffunc_nmap_func[f]);
        if (!dyna_funcs[f].handle) {
            FFUNC_ERROR("could not locate the function");
            return EXIT_FAILURE;
        }
    }
    return 1;
}

/**To prevent -std=99 issue**/
char *
ffunc_strdup(ffunc_session_t * csession, const char *str, size_t len) {
    if (len == 0 ) {
        return NULL;
    }
    char *dupstr = (char*)_ffunc_alloc(&csession->pool, len + 1);
    if (dupstr == NULL) {
        return NULL;
    }
    memset(dupstr, 0, len + 1);
    return memcpy(dupstr, str, len);
}

static void
handle_request(FCGX_Request *request) {
    char *value,
         *qparam,
         *query_string;

    ffunc_pool *p = ffunc_create_pool(DEFAULT_BLK_SZ);
    ffunc_session_t *_csession_ = (ffunc_session_t *) malloc(sizeof(ffunc_session_t));
    if (p == NULL || _csession_ == NULL) {
        char err_buff[50];
        strerror_s(err_buff, sizeof err_buff, errno);
        fprintf(stderr, "error %s\n", err_buff);
        exit(EXIT_FAILURE);
    }
    _csession_->pool = p;
    _csession_->query_str = NULL;
    _csession_->request = request;

    if ((value = FCGX_GetParam("FN_HANDLER", request->envp))) {
        int i;
        size_t vallen = strlen(value);
        for (i = 0; i < func_count; i++) {
            if ( dyna_funcs[i].len == vallen && (strcmp(dyna_funcs[i].name, value) == 0) ) {
                if ( ( qparam = _get_param_("QUERY_STRING") ) ) {
                    query_string = ffunc_strdup(_csession_, qparam, strlen(qparam));
                    if (!is_empty(query_string)) {
                        _csession_->query_str = query_string;//parse_json_fr_args(query_string);
                    }
                }
                dyna_funcs[i].handle(_csession_);

                goto RELEASE_POOL;
            }
        }
    }
    else {
        ffunc_write_out(_csession_, "Content-Type: text/plain\r\n\r\n");
        ffunc_write_out(_csession_, "%s\n", "FN_HANDLER not found");
        ffunc_write_out(_csession_, "%s\n", "Please provide your FN_HANDLER in your config file");
        ffunc_write_out(_csession_, "%s\n", "For e.g nginx.conf fastcgi_param FN_HANDLER getProfile");
    }


RELEASE_POOL:
    ffunc_destroy_pool(_csession_->pool);
    free(_csession_);
}

size_t
slen(const char* str) {
    register const char *s;
    for (s = str; *s; ++s);
    return (s - str);
}

int
ffunc_isspace(const char* s) {
    return (*s == SPACE || *s == TAB || *s == NEW_LINE ||
            *s == VERTICAL_TAB || *s == FF_FEED || *s == CARRIAGE_RETURN ? 1 : 0);
    // return (*s == '\t' || *s == '\n' ||
    //         *s == '\v' || *s == '\f' || *s == '\r' || *s == ' ' ? 1 : 0);
}

int
is_empty(char *s) {
    if (s && s[0])
        return 0;
    return 1;
}


int
strpos(const char *haystack, const char *needle)
{
    char *p = strstr(haystack, needle);
    if (p)
        return p - haystack;
    return -1;   // Not found = -1.
}

void*
ffunc_get_query_param(ffunc_session_t * csession, const char *key, size_t len) {
    int pos;
    char *qs = csession->query_str;
    if (key && *key && qs && *qs) {
        do {
            if ((pos = strpos(qs, key)) < 0) return NULL;

            if (pos == 0 || qs[pos - 1] == '&') {
                qs = (char*)qs + pos + len;
                if (*qs++ == '=') {
                    char *src = qs,
                          *ret;
                    size_t sz = 0;
                    while (*qs && *qs++ != '&')sz++;

                    ret = _ffunc_alloc(&csession->pool, sz + 1);
                    memset(ret, 0, sz + 1);
                    return memcpy(ret, src, sz);
                }
                else while (*qs && *qs++ != '&');
            }
            else while (*qs && *qs++ != '&');
        } while (*qs);
    }
    return NULL;
}

static size_t
ffunc_read_body_nolimit(ffunc_session_t * csession, ffunc_str_t *content) {
    FCGX_Request *request = csession->request;
    char* clenstr = _get_param_("CONTENT_LENGTH");
    FCGX_Stream *in = request->in;
    size_t len;

    if (clenstr && ((len = strtol(clenstr, &clenstr, 10)) != 0)) {
        content->data = _ffunc_alloc(&csession->pool, len + 1);
        memset(content->data, 0, len + 1);
        content->len = FCGX_GetStr(content->data, len, in);
    }
    else {
        content->data = NULL;
        content->len = 0;
    }

    /* Chew up whats remaining (required by mod_fastcgi) */
    while (FCGX_GetChar(in) != -1);

    return len;
}

static size_t
ffunc_read_body_limit(ffunc_session_t * csession, ffunc_str_t *content) {
    FCGX_Request *request = csession->request;
    char* clenstr = _get_param_("CONTENT_LENGTH");
    FCGX_Stream *in = request->in;
    size_t len;

    if (clenstr) {
        // long int strtol (const char* str, char** endptr, int base);
        // endptr -- Reference to an object of type char*, whose value is set by the function to the next character in str after the numerical value.
        // This parameter can also be a null pointer, in which case it is not used.
        len = strtol(clenstr, &clenstr, 10);
        /* Don't read more than the predefined limit  */
        if (len > max_std_input_buffer) {
            len = max_std_input_buffer;
        }

        content->data = _ffunc_alloc(&csession->pool, len + 1);
        memset(content->data, 0, len + 1);
        content->len = FCGX_GetStr(content->data, len, in);
    }
    /* Don't read if CONTENT_LENGTH is missing or if it can't be parsed */
    else {
        content->data = NULL;
        content->len = 0;
    }

    /* Chew up whats remaining (required by mod_fastcgi) */
    while (FCGX_GetChar(in) != -1);

    return len;
}

PROCESS_INFORMATION g_pi;

static void
ffunc_interrupt_handler(intptr_t signal) {
    TerminateProcess(g_pi.hProcess, 0);
    ExitProcess(0);
}

BOOL WINAPI
console_ctrl_handler(DWORD ctrl) {
    switch (ctrl)
    {
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        TerminateProcess(g_pi.hProcess, 0);
        return TRUE;
    default:
        return FALSE;
    }
}

/**Main api**/
int ffunc_main(int sock_port, int backlog, int max_thread, char** ffunc_nmap_func, void(*app_init_handler)(void)) {
    return ffunc_main2(sock_port, backlog, max_thread, ffunc_nmap_func, app_init_handler, 0);
}
/**Main api**/
int ffunc_main2(int sock_port, int backlog, int max_thread, char** ffunc_nmap_func, void(*app_init_handler)(void), size_t max_read_buffer) {

    if (!ffunc_nmap_func[0]) {
        ffunc_print("%s\n", "ffunc_nmap_func has no defined...");
        return 1;
    }

    if (max_read_buffer > 0) {
        max_std_input_buffer = max_read_buffer;
        ffunc_read_body = &ffunc_read_body_limit;
    }
    else {
        ffunc_read_body = &ffunc_read_body_nolimit;
    }

    ffunc_print("%s\n", "Service starting");
    ffunc_print("sock_port=%d, backlog=%d\n", sock_port, backlog);

#if defined(UNICODE) || defined(_UNICODE)
    typedef WCHAR FFUNCCMD_CHAR;
#define ffunc_cmdlen wcslen
#define ffunc_cmdstrstr wcsstr
    static const FFUNCCMD_CHAR* child_cmd_str = L"ffunc-child-proc";
#else
    typedef char FFUNCCMD_CHAR;
#define ffunc_cmdlen strlen
#define ffunc_cmdstrstr strstr
    static const FFUNCCMD_CHAR* child_cmd_str = "ffunc-child-proc";
#endif
    FFUNCCMD_CHAR *cmd_str = GetCommandLine();
    SIZE_T cmd_len = ffunc_cmdlen(cmd_str);
    SIZE_T childcmd_len = ffunc_cmdlen(child_cmd_str);
    SIZE_T spawn_child_cmd_len = cmd_len + childcmd_len + 1; // 1 for NULL terminator
 //    goto CONTINUE_CHILD_PROCESS;
    if (cmd_len > childcmd_len) {
        FFUNCCMD_CHAR *p_cmd_str = cmd_str + cmd_len - sizeof("ffunc-child-proc");
        if (ffunc_cmdstrstr(p_cmd_str, child_cmd_str)) {
			if (app_init_handler) {
				app_init_handler();
			}
            goto CONTINUE_CHILD_PROCESS;
        }
        else {
            goto SPAWN_CHILD_PROC;
        }
    }
    else {
SPAWN_CHILD_PROC:
        // Setup a console control handler: We stop the server on CTRL-C
        SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
        signal(SIGINT, ffunc_interrupt_handler);
        STARTUPINFO si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&g_pi, sizeof(g_pi));

        FFUNCCMD_CHAR *spawn_child_cmd_str = (FFUNCCMD_CHAR*)malloc(spawn_child_cmd_len * sizeof(FFUNCCMD_CHAR));
        ZeroMemory(spawn_child_cmd_str, spawn_child_cmd_len * sizeof(FFUNCCMD_CHAR));

        SIZE_T i, j;
        for (i = 0; i < cmd_len; i++) {
            spawn_child_cmd_str[i] = cmd_str[i];
        }
        spawn_child_cmd_str[i++] = ' ';

        for (j = 0; j < childcmd_len; i++, j++) {
            spawn_child_cmd_str[i] = child_cmd_str[j];
        }

        spawn_child_cmd_str[i] = '\0';

FFUNC_WORKER_RESTART:
        if (CreateProcess(
                    NULL,
                    spawn_child_cmd_str, // Child cmd string differentiate by last param
                    NULL,
                    NULL,
                    0,
                    CREATE_NO_WINDOW,
                    NULL,
                    NULL,
                    &si,
                    &g_pi) == 0) {
            FFUNC_ERROR("CreateProcess failed\n");
            Sleep(2000);
            ExitProcess(0);
        }
        fprintf(stderr, "%s\n", "Press Ctrl-C to terminate the process....");
        WaitForSingleObject(g_pi.hProcess, INFINITE);
        CloseHandle(g_pi.hProcess);
        CloseHandle(g_pi.hThread);
        if (GetLastError() != 5) {
            goto FFUNC_WORKER_RESTART;
        }
    }
    FFUNC_ERROR("Error: ");
    Sleep(3000);
    ExitProcess(0);

CONTINUE_CHILD_PROCESS:
    return hook_socket(sock_port, backlog, max_thread, ffunc_nmap_func);

}

unsigned __stdcall
ffunc_thread_worker(void* wrker) {
    int rc;
    ffunc_worker_t *worker_t = (ffunc_worker_t*)wrker;

    FCGX_Request request;
    if (FCGX_InitRequest(&request, worker_t->fcgi_func_socket, FCGI_FAIL_ACCEPT_ON_INTR) != 0) {
        ffunc_print("%s\n", "Can not init request");
        return 0;
    }
    while (1) {
        WaitForSingleObject(&worker_t->accept_mutex, INFINITE);
        rc = FCGX_Accept_r(&request);
        ReleaseMutex(&worker_t->accept_mutex);
        if (rc < 0) {
            ffunc_print("%s\n", "Cannot accept new request");
            FCGX_Finish_r(&request); // this will free all the fcgiparams memory and request
            continue;
        }
        handle_request(&request);
        FCGX_Finish_r(&request); // this will free all the fcgiparams memory and request
    }
    _endthread();
}

static int
hook_socket(int sock_port, int backlog, int max_thread, char** ffunc_nmap_func) {
    FCGX_Init();
    if (!ffunc_init(ffunc_nmap_func)) {
        exit(1);
    }

    char port_str[12];
    ffunc_worker_t *worker_t = malloc(sizeof(ffunc_worker_t));
    if (worker_t == NULL) {
        perror("nomem");
    }

    worker_t->accept_mutex = CreateMutex(NULL, FALSE, NULL); // this is lock for all threads
    int i;
    sprintf_s(port_str, sizeof port_str, ":%d", sock_port);

    if (sock_port) {
        if (backlog)
            worker_t->fcgi_func_socket = FCGX_OpenSocket(port_str, backlog);
        else
            worker_t->fcgi_func_socket = FCGX_OpenSocket(port_str, 50);
    }
    else {
        ffunc_print("%s\n", "argument wrong");
        exit(1);
    }

    if (worker_t->fcgi_func_socket < 0) {
        ffunc_print("%s\n", "unable to open the socket");
        exit(1);
    }

    ffunc_print("%d threads \n", max_thread);
    ffunc_print("Socket on hook %d\n", sock_port);

    HANDLE  *pth_workers = calloc(max_thread, sizeof(HANDLE));
    for (i = 0; i < max_thread; i++) {
        unsigned thread_id;
        pth_workers[i] = (HANDLE)_beginthreadex(NULL, 0, ffunc_thread_worker, worker_t, 0, &thread_id);
    }
    for (i = 0; i < max_thread; i++) {
        WaitForSingleObject(pth_workers[i], INFINITE);
    }
    free(pth_workers);
    ffunc_print("%s\n", "Exiting");
    return EXIT_SUCCESS;
}
#endif
#endif

void
ffunc_write_http_ok_status(ffunc_session_t * csession) {
    ffunc_write_out(csession, "Status: 200 OK\r\n");
}

void
ffunc_write_http_not_found_status(ffunc_session_t * csession) {
    ffunc_write_out(csession, "Status: 404 Not Found\r\n");
}

void
ffunc_write_http_internal_error_status(ffunc_session_t * csession) {
    ffunc_write_out(csession, "Status: 500 Internal Server Error\r\n");
}

void
ffunc_write_http_no_content_status(ffunc_session_t * csession) {
    ffunc_write_out(csession, "Status: 204 No Content\r\n");
}

void
ffunc_write_default_header(ffunc_session_t * csession) {
    ffunc_write_out(csession, "Content-Type: application/x-www-form-urlencoded\r\n\r\n");
}

void
ffunc_write_jsonp_header(ffunc_session_t * csession) {
    ffunc_write_out(csession, "Content-Type: application/javascript\r\n\r\n");
}

void
ffunc_write_json_header(ffunc_session_t * csession) {
    ffunc_write_out(csession, "Content-Type: application/json\r\n\r\n");
}

void
ffunc_destroy_pool( ffunc_pool *p ) {
    if (p) {
        if (p->prev) {
            ffunc_destroy_pool(p->prev);
        }
        free(p);
    }
}

size_t
ffunc_mem_left( ffunc_pool *p ) {
    return (uintptr_t) p->end - (uintptr_t) p->next;
}

size_t
ffunc_mem_used( ffunc_pool *p ) {
    return (uintptr_t) p->next - (uintptr_t) &p[1];
}

size_t
ffunc_blk_size( ffunc_pool *p ) {
    return (uintptr_t) p->end - (uintptr_t) (void*)&p[1];
}

static void*
mem_align(size_t size) //alignment => 16
{
    void  *p;
#ifdef POSIX_MEMALIGN
    int    err;
    err = posix_memalign(&p, ALIGNMENT, size);
    if (err) {
        ffunc_print("posix_memalign(%uz, %uz) failed \n", ALIGNMENT, size);
        p = NULL;
    }
    ffunc_print("posix_memalign: %p:%uz @%uz \n", p, size, ALIGNMENT);
#else
    // ffunc_print("%s\n", "Using Malloc");
    p = malloc(size);

#endif
    return p;
}


ffunc_pool*
ffunc_create_pool( size_t size ) {
    ffunc_pool * p = (ffunc_pool*)mem_align(size + sizeof(ffunc_pool));
    p->next = (void*)&p[1]; //p + sizeof(ffunc_pool)
    p->end = (void*)((uintptr_t)p->next + size);
    p->prev = NULL;
    return p;    // p->memblk = //mem_align( 16, DEFAULT_BLK_SZ);
}

static ffunc_pool*
ffunc_recreate_pool( ffunc_pool *curr_p, size_t new_size) {
    ffunc_pool *newp = NULL;
    if (curr_p) {
        newp = (ffunc_pool*)ffunc_create_pool(new_size);
        curr_p->next = newp->next;
        curr_p->end = newp->end;
        newp->prev = curr_p;
    }
    return newp;
}

void *
_ffunc_alloc( ffunc_pool **p, size_t size ) {
    ffunc_pool *curr_p = *p;
    if ( ffunc_mem_left(curr_p) < size ) {
        size_t curr_blk_sz = ffunc_blk_size(curr_p);
        size_t new_size = curr_blk_sz * 2;

        while ( new_size < size) {
            new_size *= 2;
        }

        *p = curr_p = ffunc_recreate_pool(curr_p, new_size);
    }
    void *mem = (void*)curr_p->next;
    curr_p->next = (void*) ((uintptr_t)curr_p->next +  size); // alloc new memory
    // memset(curr_p->next, 0, 1); // split for next

    return mem;
}



//  int main()
//  {
//      ffunc_pool *thisp = ffunc_create_pool(DEFAULT_BLK_SZ);

//      ffunc_print("thisp Address = %p\n",  thisp);
//      ffunc_print("thisp->next Address = %p\n",  thisp->next);
//      char* s = (char*)_ffunc_alloc(&thisp, DEFAULT_BLK_SZ);
//      char* s2 = (char*)_ffunc_alloc(&thisp, DEFAULT_BLK_SZ);
//      char* s3 = (char*)_ffunc_alloc(&thisp, DEFAULT_BLK_SZ);
//      ffunc_pool *thatp = ffunc_create_pool(DEFAULT_BLK_SZ);
//      char* s4 = (char*)_ffunc_alloc(&thisp, DEFAULT_BLK_SZ);
//      char* s5 = (char*)_ffunc_alloc(&thisp, DEFAULT_BLK_SZ);

//      char* s6 = (char*)_ffunc_alloc(&thatp, DEFAULT_BLK_SZ);
//      char* s7 = (char*)_ffunc_alloc(&thatp, DEFAULT_BLK_SZ);

//      ffunc_print("thisp Address = %p\n",  thisp);
//      ffunc_print("thisp->next Address = %p\n",  thisp->next);

// //     ffunc_print("thatp Address = %u\n",  thatp);
// //     ffunc_print("thatp->next Address = %u\n",  thatp->next);


//      ffunc_destroy_pool(thatp);
//      ffunc_destroy_pool(thisp);

//      return (0);
//  }
