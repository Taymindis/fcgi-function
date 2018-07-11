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
#include <setjmp.h>

#include <sys/wait.h>
#include <sys/types.h>
#include <fcgi_config.h>
#include <fcgiapp.h>
#include <dlfcn.h>
#include <pthread.h>
#include <sys/socket.h>
#include "ffunc_core.h"

#define _get_param_(KEY) FCGX_GetParam(KEY, request->envp)
#define ffunc_print(...) fprintf (stderr, __VA_ARGS__)

typedef int (*h)(FCGX_Request *, ffunc_session_t*);

struct ffunc_handler {
    char* name;
    h handle;
};

static int func_count = 0;
struct ffunc_handler *dyna_funcs = NULL;

static size_t max_std_input_buffer;
static __atomic_hash_n *thread_hash;
static int total_threads, remain_threads;

static int ffunc_init(char** ffunc_nmap_func);
void* Malloc_Function(size_t sz);
void Free_Function(void* v);
int strpos(const char *haystack, const char *needle);
static int hook_socket(int sock_port, int backlog, int max_thread, char** ffunc_nmap_func, void (*app_init_handler)(void) );
static void ffunc_add_signal_handler(void);
static void handle_request(FCGX_Request *request);
static inline void *handle_request_t(void *req) {
    handle_request((FCGX_Request*)req);
    FCGX_Finish_r((FCGX_Request*)req);
    free(req);
    __atomic_fetch_add(&remain_threads, -1, __ATOMIC_SEQ_CST);
    pthread_exit(NULL);
}
static int  has_init_signal = 0;
static struct sigaction sa;
static int fcgi_func_socket;
static void *usr_req_handle;

void* Malloc_Function(size_t sz) {
    return malloc(sz);
}
void Free_Function(void* v) {
    free(v);
}

static void*
read_t(void *origin) {
    ffunc_session_t *_csession_ = malloc(sizeof(ffunc_session_t));
    memcpy(_csession_, origin, sizeof(ffunc_session_t));
    return _csession_;
}

static void
free_t(void* node) {
    free(node);
}


static int
ffunc_init(char** ffunc_nmap_func) {
    ffunc_hash_set_alloc_fn(Malloc_Function, Free_Function);
    ffunc_buf_set_alloc_fn(Malloc_Function, Free_Function);

    usr_req_handle = NULL;
    char *error;
    usr_req_handle = dlopen(NULL, RTLD_LAZY | RTLD_NOW);
    if (!usr_req_handle) {
        ffunc_print("%s\n", dlerror());
        ffunc_print("%s\n", "Have you included -rdynamic in your compiler option, for e.g gcc ... -rdynamic");
        return 0;
    }

    while (*(ffunc_nmap_func + func_count++));

    ffunc_print("total function = %d\n", --func_count); // Remove NULL
    dyna_funcs = malloc(func_count * sizeof(struct ffunc_handler));

    for (int f = 0; f < func_count; f++) {
        dyna_funcs[f].name = calloc((strlen(ffunc_nmap_func[f]) + 1), sizeof(char));

        strcpy(dyna_funcs[f].name, ffunc_nmap_func[f]);

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
char *duplistr(const char *str) {
    int n = strlen(str) + 1;
    ffunc_session_t * csession = ffunc_get_session();
    char *dup = falloc(&csession->pool, n);
    memset(dup, 0, n);
    if (dup)
        memcpy(dup, str, n - 1);
    // dup[n - 1] = '\0';
    // please do not put ffunc_print here, it will recurve
    return dup;
}

extern void ffunc_print_time(char* buff) {
    time_t now = time(NULL);
    strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));
}

static void
handle_request(FCGX_Request *request) {

    char *value;
    char **env = request->envp;

    ffunc_pool *p = create_pool(DEFAULT_BLK_SZ);
    ffunc_session_t *_csession_ = malloc(sizeof(ffunc_session_t)), *_rel_csession_;
    __atomic_hash_n_put(thread_hash, pthread_self(), _csession_);
    _csession_->pool = p;
    _csession_->query_str = NULL;



    while (*(env) && strncmp(*env, "FN_HANDLER", 10) != 0) env++;
    value = (char*)(uintptr_t) * (env) + 11;

    if (value != NULL) {
        int i;
        for (i = 0; i < func_count; i++) {
            if (strcmp(dyna_funcs[i].name, value) == 0) {
                if (strcmp(_get_param_("REQUEST_METHOD"), "GET") == 0) {
                    char* query_string = duplistr(_get_param_("QUERY_STRING"));
                    if (!is_empty(query_string))
                        _csession_->query_str = query_string;//parse_json_fr_args(query_string);
                }

                dyna_funcs[i].handle(request, _csession_);

                goto RELEASE_POOL;
            }
        }
    } else {
        ffunc_write_out("Content-Type: text/plain\r\n\r\n");
        ffunc_write_out("%s\n", "FN_HANDLER not found");
        ffunc_write_out("%s\n", "Please provide your FN_HANDLER in your config file");
        ffunc_write_out("%s\n", "For e.g nginx.conf fastcgi_param FN_HANDLER getProfile");
    }


RELEASE_POOL:
    _rel_csession_ = __atomic_hash_n_pop(thread_hash, pthread_self());
    if (_rel_csession_) { //free the _csession_
        destroy_pool(_rel_csession_->pool);
        free(_rel_csession_);
        // fprintf(stdout, "%s\n", "successfully free _csession_:");
        // fprintf(stdout, "%s\n", "successfully freed");
        // so far all are freed.
        // fhash_unlock(thread_table);
    } else {
        destroy_pool(p);
        // fhash_unlock(thread_table);
    }
}

size_t slen(const char* str) {
    register const char *s;
    for (s = str; *s; ++s);
    return (s - str);
}

int ffunc_isspace(const char* s) {
    return (*s == SPACE || *s == TAB || *s == NEW_LINE ||
            *s == VERTICAL_TAB || *s == FF_FEED || *s == CARRIAGE_RETURN ? 1 : 0);
    // return (*s == '\t' || *s == '\n' ||
    //         *s == '\v' || *s == '\f' || *s == '\r' || *s == ' ' ? 1 : 0);
}

int is_empty(char *s) {
    if (s && s[0])
        return 0;
    return 1;
}

int ffunc_file_existd(const char* fname) {
    if ( access( fname, F_OK ) != -1 )
        return 1;
    else
        return 0;
}

int strpos(const char *haystack, const char *needle)
{
    char *p = strstr(haystack, needle);
    if (p)
        return p - haystack;
    return -1;   // Not found = -1.
}

void* ffunc_getParam(const char *key, char* query_str) {
    int len, pos;
    char *qs = query_str;
    if (key && *key && qs && *qs) {
        len = strlen(key);
        do {
            if ((pos = strpos(qs, key)) < 0) return NULL;

            if (pos == 0 || qs[pos - 1] == '&') {
                qs = (char*)qs + pos + len;
                if (*qs++ == '=') {
                    char *src = qs,
                          *ret;
                    size_t sz = 0;
                    while (*qs && *qs++ != '&')sz++;

                    ffunc_session_t * csession = ffunc_get_session();
                    ret = falloc(&csession->pool, sz + 1);
                    memset(ret, 0, sz + 1);
                    return memcpy(ret, src, sz);
                } else while (*qs && *qs++ != '&');
            } else while (*qs && *qs++ != '&');
        } while (*qs);
    }
    return NULL;
}

long ffunc_readContent(FCGX_Request *request, char** content) {
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

        ffunc_session_t * csession = ffunc_get_session();
        *content = falloc(&csession->pool, len + 1);
        memset(*content, 0, len + 1);
        len = FCGX_GetStr(*content, len, in);
    }
    /* Don't read if CONTENT_LENGTH is missing or if it can't be parsed */
    else {
        *content = 0;
        len = 0;
    }

    /* Chew up whats remaining (required by mod_fastcgi) */
    while (FCGX_GetChar(in) != -1);

    return len;
}

ffunc_session_t *ffunc_get_session(void) {
    return (ffunc_session_t *)__atomic_hash_n_get(thread_hash, pthread_self());
}

/**Main api**/
int
ffunc_main(int sock_port, int backlog, int max_thread, char** ffunc_nmap_func, void (*app_init_handler)(void)) {
    return ffunc_main2(sock_port, backlog, max_thread, ffunc_nmap_func, app_init_handler, 10240);
}
/**Main api**/
int
ffunc_main2(int sock_port, int backlog, int max_thread, char** ffunc_nmap_func, void (*app_init_handler)(void), size_t max_read_buffer) {
    pid_t childPID;
    int child_status;
    max_std_input_buffer = max_read_buffer;

    if (!ffunc_nmap_func[0]) {
        ffunc_print("%s\n", "ffunc_nmap_func has no defined...");
        return 1;
    }

    ffunc_print("%s\n", "Service starting");
    ffunc_print("sock_port=%d, backlog=%d\n", sock_port, backlog);

FFUNC_WORKER_RESTART:
    if (!has_init_signal) {
        ffunc_add_signal_handler();
    }
    childPID = fork();
    if (childPID >= 0) {// fork was successful
        if (childPID == 0) {// child process
            return hook_socket(sock_port, backlog, max_thread, ffunc_nmap_func, app_init_handler);
        } else {//Parent process
            while (1) {
                if (waitpid(childPID, &child_status, WNOHANG) == childPID) {
                    has_init_signal = 0;
                    goto FFUNC_WORKER_RESTART;
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

static int
hook_socket(int sock_port, int backlog, int max_thread, char** ffunc_nmap_func, void (*app_init_handler)(void)) {
    if (app_init_handler) {
        app_init_handler();
    }
    FCGX_Init();
    if (!ffunc_init(ffunc_nmap_func)) {
        exit(1);
    }

    char port_str[12];
    sprintf(port_str, ":%d", sock_port);
    remain_threads = 0;
    total_threads = max_thread;

    if (sock_port) {
        if (backlog)
            fcgi_func_socket = FCGX_OpenSocket(port_str, backlog);
        else
            fcgi_func_socket = FCGX_OpenSocket(port_str, 50);
    } else {
        ffunc_print("%s\n", "argument wrong");
        exit(1);
    }

    if (fcgi_func_socket < 0) {
        ffunc_print("%s\n", "unable to open the socket" );
        exit(1);
    }

    ffunc_print("%d threads \n", max_thread);
    thread_hash = __atomic_hash_n_init(max_thread + 10, read_t, free_t);
    ffunc_print("Socket on hook %d\n", sock_port);


    FCGX_Request *request = calloc(1, sizeof(FCGX_Request));
    FCGX_InitRequest(request, fcgi_func_socket, FCGI_FAIL_ACCEPT_ON_INTR);

    while (FCGX_Accept_r(request) >= 0) {
        if (__atomic_fetch_add(&remain_threads, 1, __ATOMIC_SEQ_CST) <= total_threads) {
            pthread_t t;
            if (pthread_create(&t, NULL, handle_request_t, request)) {
                FCGX_Finish_r((FCGX_Request*)request);
                free(request);
                fprintf(stderr, "Error creating thread %s\n", strerror(errno));
            }
            pthread_detach(t);
            // handle_request(request);
            // FCGX_Finish_r(request);
        } else {
            ffunc_write_http_status(request, 500);
            FCGX_FPrintF(request->out, "Content-Type: text/plain\r\n\r\n");
            FCGX_FPrintF(request->out, "%s\n", "Out of acceptable threads ");
            FCGX_Finish_r(request);
            free(request);
            __atomic_fetch_add(&remain_threads, -1, __ATOMIC_SEQ_CST);
        }
        request = calloc(1, sizeof(FCGX_Request));
        FCGX_InitRequest(request, fcgi_func_socket, FCGI_FAIL_ACCEPT_ON_INTR);
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


void
ffunc_write_http_status(FCGX_Request * request, uint16_t code) {

    switch (code) {
    case 200:
        ffunc_write_out("Status: 200 OK\r\n");
        break;
    case 204:
        ffunc_write_out("Status: 204 No Content\r\n");
        break;

    case 500:
        ffunc_write_out("Status: 500 Internal Server Error\r\n");
        break;

    default:
        ffunc_write_out("Status: 200 OK\r\n");
        break;
    }
}

void ffunc_write_default_header(FCGX_Request * request) {
    ffunc_write_out("Content-Type: application/x-www-form-urlencoded\r\n\r\n");
}

void ffunc_write_jsonp_header(FCGX_Request * request) {
    ffunc_write_out("Content-Type: application/javascript\r\n\r\n");
}

void ffunc_write_json_header(FCGX_Request * request) {
    ffunc_write_out("Content-Type: application/json\r\n\r\n");
}
