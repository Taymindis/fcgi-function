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

#define _FFUNC_MASTER_ENV "_ffunc-master_" __TIME__
#define FFUNC_SETENV setenv
#define FFUNC_GETENV getenv
#define FFUNC_EXECVE execve
#define _FFUNC_MASTER_ " ffunc-master"
#define _FFUNC_WORKER_ " ffunc-worker"

static void* mem_align(size_t size);
static int ffunc_get_number_of_digit(long long number);
static ffunc_pool* ffunc_recreate_pool(ffunc_pool *curr_p, size_t new_size);
static ffunc_str_t ffunc_proc_name;

typedef void(*h)(ffunc_session_t*);

struct ffunc_handler {
    char* name;
    size_t len;
    h handle;
};

typedef struct {
    int fcgi_func_socket;
    pthread_mutex_t accept_mutex;
} ffunc_worker_t;

static int func_count = 0;
struct ffunc_handler *dyna_funcs = NULL;

static size_t max_std_input_buffer;

static int ffunc_init(char** ffunc_nmap_func);
static int ffunc_strpos(const char *haystack, const char *needle);
void *ffunc_thread_worker(void* wrker);
static int hook_socket(int sock_port, char* sock_port_str, int backlog, int max_thread, char** ffunc_nmap_func, int* procpip);
static void ffunc_add_signal_handler(void);
static void handle_request(FCGX_Request *request);
static size_t ffunc_read_body_limit(ffunc_session_t * csession, ffunc_str_t *content);
static size_t ffunc_read_body_nolimit(ffunc_session_t * csession, ffunc_str_t *content);
static void ffunc_default_direct_consume(ffunc_session_t *sess);
static int  has_init_signal = 0;
static struct sigaction sa;
static void *usr_req_handle;
// extern declaration
size_t(*ffunc_read_body)(ffunc_session_t *, ffunc_str_t *);
void(*_ffunc_direct_consume)(ffunc_session_t *);

static int
ffunc_init(char** ffunc_nmap_func) {
    int f = 0;
    size_t szof_func_name;
    usr_req_handle = NULL;
    char *error;
    usr_req_handle = dlopen(NULL, RTLD_LAZY | RTLD_NOW);
    if (!usr_req_handle) {
        ffunc_print("%s\n", dlerror());
        ffunc_print("%s\n", "Module not found");
        return 0;
    }

    for (func_count = 0; ffunc_nmap_func[func_count]; func_count++);

    dyna_funcs = (struct ffunc_handler*) malloc(func_count * sizeof(struct ffunc_handler));
    for (f = 0; f < func_count; f++) {
        // szof_func_name = strlen(ffunc_nmap_func[f]);
        dyna_funcs[f].name = ffunc_nmap_func[f]; //(char*) calloc(szof_func_name + 1, sizeof(char));

                                                 // memcpy(dyna_funcs[f].name, ffunc_nmap_func[f], szof_func_name);
        dyna_funcs[f].len = strlen(ffunc_nmap_func[f]);
        *(void **)(&dyna_funcs[f].handle) = dlsym(usr_req_handle, ffunc_nmap_func[f]);

        if ((error = dlerror()) != NULL) {
            ffunc_print("%s\n", error);
            ffunc_print("%s\n", "Have you included -rdynamic in your compiler option, for e.g gcc ... -rdynamic");
            return 0;
        }
    }

    *(void **)(&_ffunc_direct_consume) = dlsym(usr_req_handle, "ffunc_direct_consume");
    if ((error = dlerror()) != NULL) {
        ffunc_print("%s\n", "direct consume disabled");
        _ffunc_direct_consume = &ffunc_default_direct_consume;
    }
    else {
        ffunc_print("%s\n", "direct consume enabled");
    }
    return 1;
}

/**To prevent -std=99 issue**/
char *
ffunc_strdup(ffunc_session_t * csession, const char *str, size_t len) {
    if (len == 0) {
        return NULL;
    }
    char *dupstr = (char*)_ffunc_alloc(&csession->pool, len + 1);
    if (dupstr == NULL) {
        return NULL;
    }
    memcpy(dupstr, str, len);
    // memset(dupstr, 0, len + 1);
    dupstr[len] = (char)0;
    return dupstr;
}

static void
handle_request(FCGX_Request *request) {
    char *value,
        *qparam,
        *query_string;

    ffunc_pool *p = ffunc_create_pool(DEFAULT_BLK_SZ);
    ffunc_session_t *_csession_ = (ffunc_session_t*)malloc(sizeof(ffunc_session_t));
    if (p == NULL || _csession_ == NULL) {
        fprintf(stderr, "error %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    _csession_->pool = p;
    _csession_->query_str = NULL;
    _csession_->request = request;

    if ((value = FCGX_GetParam("FN_HANDLER", request->envp))) {
        int i;
        size_t vallen = strlen(value);
        for (i = 0; i < func_count; i++) {
            if (dyna_funcs[i].len == vallen && (strcmp(dyna_funcs[i].name, value) == 0)) {
                if ((qparam = _get_param_("QUERY_STRING"))) {
                    query_string = ffunc_strdup(_csession_, qparam, strlen(qparam));
                    if (query_string && query_string[0]) {
                        _csession_->query_str = query_string;//parse_json_fr_args(query_string);
                    }
                }
                dyna_funcs[i].handle(_csession_);

                goto RELEASE_POOL;
            }
        }
        ffunc_default_direct_consume(_csession_);
    }
    else {
        if ((qparam = _get_param_("QUERY_STRING"))) {
            query_string = ffunc_strdup(_csession_, qparam, strlen(qparam));
            if (query_string && query_string[0]) {
                _csession_->query_str = query_string;//parse_json_fr_args(query_string);
            }
        }
        _ffunc_direct_consume(_csession_);
    }

RELEASE_POOL:
    ffunc_destroy_pool(_csession_->pool);
    free(_csession_);
}

static size_t
ffunc_read_body_nolimit(ffunc_session_t * csession, ffunc_str_t *content) {
    FCGX_Request *request = csession->request;
    char* clenstr = _get_param_("CONTENT_LENGTH");
    FCGX_Stream *in = request->in;
    size_t len;

    if (clenstr && ((len = strtol(clenstr, &clenstr, 10)) != 0)) {
        content->data = (char*)_ffunc_alloc(&csession->pool, len + 1);
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
        if (len > max_std_input_buffer) { len = max_std_input_buffer; }

        content->data = (char*)_ffunc_alloc(&csession->pool, len + 1);
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

/**Main Hook**/
int
ffunc_hook(ffunc_config_t *conf) {
    pid_t child_pid, daemon_pid;
    int child_status;
    int procpip[2];
    char pipbuf[FFUNC_APP_INIT_PIPE_BUF_SIZE];
    int sock_port = conf->sock_port;
    char* sock_port_str = conf->sock_port_str;
    int backlog = conf->backlog;
    int max_thread = conf->max_thread;
    char** ffunc_nmap_func = conf->ffunc_nmap_func;
    size_t max_read_buffer = conf->max_read_buffer;

    if (max_read_buffer > 0) {
        max_std_input_buffer = max_read_buffer;
        ffunc_read_body = &ffunc_read_body_limit;
    }
    else {
        ffunc_read_body = &ffunc_read_body_nolimit;
    }

    if (sock_port <= 0 && sock_port_str == NULL) {
        ffunc_print("%s\n", "sock_port has no defined...");
        return 1;
    }

    if (max_thread <= 0) {
        ffunc_print("%s\n", "max_thread has no defined...");
        return 1;
    }

    if (backlog <= 0) {
        ffunc_print("%s\n", "backlog has no defined...");
        return 1;
    }

    if (!ffunc_nmap_func[0]) {
        ffunc_print("%s\n", "ffunc_nmap_func has no defined...");
        return 1;
    }

    ffunc_print("%s\n", "Service starting");
	if (sock_port) {
		ffunc_print("sock_port=%d, backlog=%d\n", sock_port, backlog);
	} else if(sock_port_str) {
		ffunc_print("sock_port=%s, backlog=%d\n", sock_port_str, backlog);
	}
    /** Do master pipe before fork **/
    if (pipe(procpip) < 0)
        exit(1);

FFUNC_WORKER_RESTART:
    if (conf->daemon) {
        daemon_pid = fork();

        /* An error occurred */
        if (daemon_pid < 0)
            exit(EXIT_FAILURE);

        /* Success: Let the parent terminate */
        if (daemon_pid > 0)
            exit(EXIT_SUCCESS);

        /* On success: The child process becomes session leader */
        if (setsid() < 0)
            exit(EXIT_FAILURE);
    }

    if (!has_init_signal) {
        ffunc_add_signal_handler();
    }
    write(procpip[1], FFUNC_APP_INITIALIZING, FFUNC_APP_INIT_PIPE_BUF_SIZE);
    child_pid = fork();
    if (child_pid >= 0) {// fork was successful
        if (child_pid == 0) {// child process
            char *swap_name = (ffunc_proc_name.data + ffunc_proc_name.len) - (sizeof(_FFUNC_MASTER_) - 1);
            memcpy(swap_name, _FFUNC_WORKER_, (sizeof(_FFUNC_MASTER_) - 1));
            if (conf->app_init_handler) {
                conf->app_init_handler();
            }
            return hook_socket(sock_port, sock_port_str, backlog, max_thread, ffunc_nmap_func, procpip);
        }
        else { // Parent process
            while (1) {
                if (waitpid(child_pid, &child_status, WNOHANG) == child_pid) {
                    has_init_signal = 0;
                    if (read(procpip[0], pipbuf, FFUNC_APP_INIT_PIPE_BUF_SIZE) > 0) {
                        if (strncmp(pipbuf, FFUNC_APP_INITIALIZED, FFUNC_APP_INIT_PIPE_BUF_SIZE) == 0) {
                            goto FFUNC_WORKER_RESTART;
                        }
                        else {
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
    }
    else {// fork failed
        ffunc_print("%s\n", "Fork Child failed..");
        return -1;
    }
    return 0;
    // return hook_socket(sock_port, backlog, max_thread, ffunc_nmap_func, app_init_handler);
}

void *
ffunc_thread_worker(void* wrker) {
    int rc;
    ffunc_worker_t *worker_t = (ffunc_worker_t*)wrker;

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
hook_socket(int sock_port, char *sock_port_str, int backlog, int max_thread, char** ffunc_nmap_func, int* procpip) {
    char pipbuf[FFUNC_APP_INIT_PIPE_BUF_SIZE];
    FCGX_Init();
    if (!ffunc_init(ffunc_nmap_func)) {
        exit(1);
    }

    char port_str[12];
    ffunc_worker_t *worker_t = (ffunc_worker_t*)malloc(sizeof(ffunc_worker_t));
    if (worker_t == NULL) {
        perror("nomem");
    }

    worker_t->accept_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER; // this is lock for all threads
    int i;

    if (sock_port) {
        sprintf(port_str, ":%d", sock_port);
        if (backlog)
            worker_t->fcgi_func_socket = FCGX_OpenSocket(port_str, backlog);
        else
            worker_t->fcgi_func_socket = FCGX_OpenSocket(port_str, 50);
    } else if(sock_port_str) {
      if (backlog)
          worker_t->fcgi_func_socket = FCGX_OpenSocket(sock_port_str, backlog);
      else
          worker_t->fcgi_func_socket = FCGX_OpenSocket(sock_port_str, 50);
    } else {
        ffunc_print("%s\n", "argument wrong");
        exit(1);
    }

    if (worker_t->fcgi_func_socket < 0) {
        ffunc_print("%s\n", "unable to open the socket");
        exit(1);
    }

    ffunc_print("%d threads \n", max_thread);

    /** Release for success initialized **/
    read(procpip[0], pipbuf, FFUNC_APP_INIT_PIPE_BUF_SIZE);
    write(procpip[1], FFUNC_APP_INITIALIZED, FFUNC_APP_INIT_PIPE_BUF_SIZE);
    close(procpip[0]);
    close(procpip[1]);

    pthread_t pth_workers[max_thread];
    for (i = 0; i < max_thread; i++) {
        pthread_create(&pth_workers[i], NULL, ffunc_thread_worker, worker_t);
        // pthread_detach(pth_worker);
    }
    for (i = 0; i < max_thread; i++) {
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

int
main(int argc, char *argv[]) {
    ffunc_config_t *conf;
    char **new_argv, *env_ffunc_master;
    unsigned int i;
    int status;

    if (FFUNC_GETENV(_FFUNC_MASTER_ENV)) {
        conf = (ffunc_config_t*)calloc(1, sizeof(ffunc_config_t));
        conf->sock_port_str = NULL;
        ffunc_proc_name.data = argv[0];
        ffunc_proc_name.len = strlen(argv[0]);

        if ((status = ffunc_main(argc, argv, conf)) == 0) {
            return ffunc_hook(conf);
        }
        else {
            fprintf(stderr, "Error status %d, status return is not successful(0) \n", status);
            return status;
        }
    }
    else {
        FFUNC_SETENV(_FFUNC_MASTER_ENV, argv[0], 1);
        new_argv = (char **)calloc(argc + 1, sizeof(char*));
        size_t arg$0sz = strlen(argv[0]);
        new_argv[0] = (char *)calloc(arg$0sz + sizeof(_FFUNC_MASTER_), sizeof(char));
        memcpy(new_argv[0], argv[0], arg$0sz);
        memcpy(new_argv[0] + arg$0sz, _FFUNC_MASTER_, sizeof(_FFUNC_MASTER_) - 1);

        for (i = 1; i < argc; i++) {
            new_argv[i] = argv[i];
        }

        new_argv[i] = NULL;
        extern char **environ;
        FFUNC_EXECVE(argv[0], new_argv, environ);
    }
    return 0;
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


#ifdef  UNICODE
#define FFUNC_SPRINTF swprintf
#define FFUNC_PRINTF wprintf
#else
#define FFUNC_SPRINTF sprintf_s
#define FFUNC_PRINTF printf
#endif
#define ffunc_spawn_child _spawnve
#define pipename TEXT("\\\\.\\pipe\\LogPipe")
static TCHAR *fullPipeName = NULL;
#define NAMED_PIPED_BUFF_SIZE 32
#define _get_param_(KEY) FCGX_GetParam(KEY, request->envp)
#define FFUNC_ERROR(errmsg) FFUNC_PRINTF(TEXT("%s - %d\n"), TEXT(errmsg), GetLastError() )

int setenv(const char *name, const char *value, int overwrite) {
    int errcode = 0;
    if (!overwrite) {
        size_t envsize = 0;
        errcode = getenv_s(&envsize, NULL, 0, name);
        if (errcode || envsize) return errcode;
    }
    return _putenv_s(name, value);
}
char* getenv_secure(char const* _VarName) {
    size_t envsize = 0;
    char *exec_name = NULL;
    if (getenv_s(&envsize, NULL, 0, _VarName) || envsize <= 0) {
        return NULL;
    }
    exec_name = calloc(envsize + 1, sizeof(char));
    if (getenv_s(&envsize, exec_name, envsize, _VarName) || envsize <= 0) {
        return NULL;
    }
    return exec_name;
}
#define _FFUNC_MASTER_ENV "_ffunc-master_" __TIME__
#define FFUNC_SETENV setenv
#define FFUNC_GETENV(varname) getenv_secure(varname)
#define FFUNC_EXECVE _spawnve
#define _FFUNC_MASTER_ "_ffunc-master"
#define _FFUNC_WORKER_ "_ffunc-worker"

static void* mem_align(size_t size);
static int ffunc_get_number_of_digit(long long number);
static ffunc_pool* ffunc_recreate_pool(ffunc_pool *curr_p, size_t new_size);
typedef void(*h)(ffunc_session_t*);
typedef void(*inithandler)(void);
static ffunc_str_t ffunc_proc_name;

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

static int ffunc_init(char** ffunc_nmap_func, char* init_handler_name);
static int ffunc_strpos(const char *haystack, const char *needle);
unsigned __stdcall ffunc_thread_worker(void *wrker);
static int hook_socket(int sock_port, char* sock_port_str, int backlog, int max_thread, char** ffunc_nmap_func, char* init_handler_name);
static void ffunc_add_signal_handler(void);
static void handle_request(FCGX_Request *request);
static size_t ffunc_read_body_limit(ffunc_session_t * csession, ffunc_str_t *content);
static size_t ffunc_read_body_nolimit(ffunc_session_t * csession, ffunc_str_t *content);
static void ffunc_default_direct_consume(ffunc_session_t *sess);
static int  has_init_signal = 0;
static HINSTANCE usr_req_handle;

// extern declaration
size_t(*ffunc_read_body)(ffunc_session_t *, ffunc_str_t *);
h _ffunc_direct_consume;

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
ffunc_init(char** ffunc_nmap_func, char* init_handler_name) {
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

        dyna_funcs[f].handle = (h)GetProcAddress(usr_req_handle, ffunc_nmap_func[f]);
        if (!dyna_funcs[f].handle) {
            FFUNC_ERROR("could not locate the function");
            return EXIT_FAILURE;
        }
    }



    _ffunc_direct_consume = (h)GetProcAddress(usr_req_handle, "ffunc_direct_consume");
    if (!_ffunc_direct_consume) {
        FFUNC_PRINTF(TEXT("direct consume is disabled\n"));
        _ffunc_direct_consume = &ffunc_default_direct_consume;
    }
    else {
        FFUNC_PRINTF(TEXT("direct consume is enabled\n"));
    }

	if (init_handler_name) {
		inithandler initHandler = (inithandler)GetProcAddress(usr_req_handle, init_handler_name);
		if (initHandler) {
			initHandler();
		}
	}

    return 1;
}

/**To prevent -std=99 issue**/
char *
ffunc_strdup(ffunc_session_t * csession, const char *str, size_t len) {
    if (len == 0) {
        return NULL;
    }
    char *dupstr = (char*)_ffunc_alloc(&csession->pool, len + 1);
    if (dupstr == NULL) {
        return NULL;
    }
    memcpy(dupstr, str, len);
    // memset(dupstr, 0, len + 1);
    dupstr[len] = (char)0;
    return dupstr;
}

static void
handle_request(FCGX_Request *request) {
    char *value,
        *qparam,
        *query_string;

    ffunc_pool *p = ffunc_create_pool(DEFAULT_BLK_SZ);
    ffunc_session_t *_csession_ = (ffunc_session_t *)malloc(sizeof(ffunc_session_t));
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
            if (dyna_funcs[i].len == vallen && (strcmp(dyna_funcs[i].name, value) == 0)) {
                if ((qparam = _get_param_("QUERY_STRING"))) {
                    query_string = ffunc_strdup(_csession_, qparam, strlen(qparam));
                    if (query_string && query_string[0]) {
                        _csession_->query_str = query_string;//parse_json_fr_args(query_string);
                    }
                }
                dyna_funcs[i].handle(_csession_);

                goto RELEASE_POOL;
            }
        }
        ffunc_default_direct_consume(_csession_);
    }
    else {
        if ((qparam = _get_param_("QUERY_STRING"))) {
            query_string = ffunc_strdup(_csession_, qparam, strlen(qparam));
            if (query_string && query_string[0]) {
                _csession_->query_str = query_string;//parse_json_fr_args(query_string);
            }
        }
        _ffunc_direct_consume(_csession_);
    }


RELEASE_POOL:
    ffunc_destroy_pool(_csession_->pool);
    free(_csession_);
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


static void
ffunc_interrupt_handler(intptr_t signal) {
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
        return TRUE;
    default:
        return FALSE;
    }
}

/**Main Hook**/
int ffunc_hook(char **argv, ffunc_config_t *conf, int bypassmaster) {
    int sock_port = conf->sock_port;
    char* sock_port_str = conf->sock_port_str;
    int backlog = conf->backlog;
    int max_thread = conf->max_thread;
    char** ffunc_nmap_func = conf->ffunc_nmap_func;
	char* app_init_handler_name = conf->app_init_handler_name;
	size_t max_read_buffer = conf->max_read_buffer;
    char* exec_name = conf->__exec_name;
    int totalPipeLength;
    TCHAR *pipenamebuff;


    if (sock_port <= 0 && sock_port_str == NULL) {
        FFUNC_PRINTF(TEXT("%s\n"), TEXT("sock_port has no defined..."));
        return 1;
    }

    if (max_thread <= 0) {
        FFUNC_PRINTF(TEXT("%s\n"), TEXT("max_thread has no defined..."));
        return 1;
    }

    if (backlog <= 0) {
        FFUNC_PRINTF(TEXT("%s\n"), TEXT("backlog has no defined..."));
        return 1;
    }

    if (!ffunc_nmap_func[0]) {
        FFUNC_PRINTF(TEXT("%s\n"), TEXT("ffunc_nmap_func has no defined..."));
        return 1;
    }

    if (max_read_buffer > 0) {
        max_std_input_buffer = max_read_buffer;
        ffunc_read_body = &ffunc_read_body_limit;
    }
    else {
        ffunc_read_body = &ffunc_read_body_nolimit;
    }

    if (bypassmaster) {
        goto CONTINUE_CHILD_PROCESS;
    }
    else {
        goto SPAWN_CHILD_PROC;
    }

SPAWN_CHILD_PROC:
    // Setup a console control handler: We stop the server on CTRL-C
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
    signal(SIGINT, ffunc_interrupt_handler);

    SIZE_T i;
    char *sockport_str,
        *backlog_str,
        *max_thread_str,
        *max_read_buffer_str;

    DWORD parentPid = GetCurrentProcessId();
    HANDLE pipe;

    if (parentPid < 0) {
        FFUNC_PRINTF(TEXT("%s\n"), TEXT("invalid pid"));
        return 1;
    }
    else {
        totalPipeLength = ffunc_get_number_of_digit(parentPid) + sizeof(pipename);
        pipenamebuff = (TCHAR*)calloc(totalPipeLength, sizeof(TCHAR));
        FFUNC_SPRINTF(pipenamebuff, totalPipeLength, TEXT("%s%d"), pipename, parentPid);
        pipe = CreateNamedPipe(pipenamebuff, PIPE_ACCESS_INBOUND | PIPE_ACCESS_OUTBOUND, PIPE_WAIT, 1,
            NAMED_PIPED_BUFF_SIZE, NAMED_PIPED_BUFF_SIZE, 120 * 1000, NULL);
        if (pipe == INVALID_HANDLE_VALUE) {
            FFUNC_PRINTF(TEXT("Error: %d"), GetLastError());
        }
#ifdef UNICODE
        char *toCharPipe = calloc(totalPipeLength + 1, sizeof(char));
        size_t   i;
        wcstombs_s(&i, toCharPipe, totalPipeLength, pipenamebuff, totalPipeLength);
        FFUNC_SETENV("_FFUNC_PID_NAMED_PIPE", toCharPipe, 1);
#else
        FFUNC_SETENV("_FFUNC_PID_NAMED_PIPE", pipenamebuff, 1);
#endif
    }

    if (sock_port != 0) {
        sockport_str = malloc((ffunc_get_number_of_digit(sock_port) + 1) * sizeof(char));
        snprintf(sockport_str, ffunc_get_number_of_digit(sock_port) + 1, "%d", sock_port);
        FFUNC_SETENV("_FFUNC_ENV_SOCK_PORT", sockport_str, 1);
    } else if (sock_port_str != NULL) {
        sockport_str = calloc((strlen(sock_port_str) + 1), sizeof(char));
        memcpy(sockport_str, sock_port_str, strlen(sock_port_str));
        FFUNC_SETENV("_FFUNC_ENV_SOCK_PORT_STR", sockport_str, 1);
    }

	if (app_init_handler_name) {
		FFUNC_SETENV("_FFUNC_ENV_INIT_HANDLER_NAME", app_init_handler_name, 1);
	}

    if (backlog != 0) {
        backlog_str = malloc((ffunc_get_number_of_digit(backlog) + 1) * sizeof(char));
        snprintf(backlog_str, ffunc_get_number_of_digit(backlog) + 1, "%d", backlog);
        FFUNC_SETENV("_FFUNC_ENV_BACKLOG", backlog_str, 1);
    }

    if (max_thread != 0) {
        max_thread_str = malloc((ffunc_get_number_of_digit(max_thread) + 1) * sizeof(char));
        snprintf(max_thread_str, ffunc_get_number_of_digit(max_thread) + 1, "%d", max_thread);
        FFUNC_SETENV("_FFUNC_ENV_MAX_THREAD", max_thread_str, 1);
    }


    if (max_read_buffer != 0) {
        max_read_buffer_str = malloc((ffunc_get_number_of_digit(max_read_buffer) + 1) * sizeof(char));
        snprintf(max_read_buffer_str, ffunc_get_number_of_digit(max_read_buffer) + 1, "%d", max_read_buffer);
        FFUNC_SETENV("_FFUNC_ENV_MAX_READ_BUF", max_read_buffer_str, 1);
    }


    SIZE_T func_str_sz = 0;
    for (i = 0; ffunc_nmap_func[i]; i++) {
        func_str_sz = func_str_sz + strlen(ffunc_nmap_func[i]) + sizeof("\",\" ") - 1;
    }

    char *func_str = malloc((func_str_sz + 1) * sizeof(char));
    char* p = func_str;
    for (i = 0; ffunc_nmap_func[i]; i++) {
        p = (char*)(memcpy(p, ffunc_nmap_func[i], strlen(ffunc_nmap_func[i]))) + strlen(ffunc_nmap_func[i]);
        p = (char*)(memcpy(p, "\",\" ", sizeof("\",\" ") - 1)) + (sizeof("\",\" ") - 1);
    }
    func_str[func_str_sz] = (char)0;

    FFUNC_SETENV("_FFUNC_ENV_FUNC_str", func_str, 1);
    // FFUNC_PRINTF("func_str = %s\n", func_str);

    // Change command line to worker
    if ((strlen(exec_name) + (sizeof(_FFUNC_WORKER_) - 1)) <= strlen(argv[0])) {
        memmove(argv[0] + strlen(exec_name), _FFUNC_WORKER_, (sizeof(_FFUNC_WORKER_) - 1));
    }
    extern char **environ;

FFUNC_WORKER_RESTART:
    if (exec_name) {
        FFUNC_EXECVE(_P_WAIT, exec_name, argv, environ);
        char data[NAMED_PIPED_BUFF_SIZE];
        DWORD numRead;
        //ConnectNamedPipe(pipe, NULL);
        if (PeekNamedPipe(pipe, NULL, 0, NULL, &numRead, NULL)) {
            if (0 != numRead) {
                ReadFile(pipe, data, 1024, &numRead, NULL);
                goto FFUNC_WORKER_RESTART;
            }
        }

        CloseHandle(pipe);

    }
    else {
        FFUNC_ERROR("Error: Exec name not found ");

    }

    FFUNC_ERROR("Error: ");
    Sleep(3000);
    ExitProcess(0);

CONTINUE_CHILD_PROCESS:

    FFUNC_PRINTF(TEXT("%s\n"), TEXT("Service starting"));
	if (sock_port) {
		FFUNC_PRINTF(TEXT("Socket port on hook %d\n"), sock_port);
	}
	else if(sock_port_str){
		FFUNC_PRINTF(TEXT("Socket port str on hook %s\n"), sock_port_str);
	}
    FFUNC_PRINTF(TEXT("backlog=%d\n"), backlog);
    FFUNC_PRINTF(TEXT("%d threads \n"), max_thread);
    fprintf(stderr, "%s\n", "Press Ctrl-C to terminate the process....");
    return hook_socket(sock_port, sock_port_str, backlog, max_thread, ffunc_nmap_func, conf->app_init_handler_name);
}

unsigned __stdcall
ffunc_thread_worker(void* wrker) {
    int rc;
    ffunc_worker_t *worker_t = (ffunc_worker_t*)wrker;

    FCGX_Request request;
    if (FCGX_InitRequest(&request, worker_t->fcgi_func_socket, FCGI_FAIL_ACCEPT_ON_INTR) != 0) {
        FFUNC_PRINTF(TEXT("%s\n"), TEXT("Can not init request"));
        return 0;
    }
    while (1) {
        WaitForSingleObject(&worker_t->accept_mutex, INFINITE);
        rc = FCGX_Accept_r(&request);
        ReleaseMutex(&worker_t->accept_mutex);
        if (rc < 0) {
            FFUNC_PRINTF(TEXT("%s\n"), TEXT("Cannot accept new request"));
            FCGX_Finish_r(&request); // this will free all the fcgiparams memory and request
            continue;
        }
        handle_request(&request);
        FCGX_Finish_r(&request); // this will free all the fcgiparams memory and request
    }
    _endthread();
}

static int
hook_socket(int sock_port, char* sock_port_str, int backlog, int max_thread, char** ffunc_nmap_func, char* init_handler_name) {
    char *_pipeName;
    // #define MAX_PIPE_SIZE 256
    // TCHAR *pipeName;
    HANDLE pipe;
    if (!(_pipeName = FFUNC_GETENV("_FFUNC_PID_NAMED_PIPE"))) {
        FFUNC_PRINTF(TEXT("%s\n"), TEXT("Failed getting _FFUNC_PID_NAMED_PIPE"));
        return -1;
    }

    /*
    #ifdef UNICODE
    size_t szi;
    pipeName = calloc(MAX_PIPE_SIZE, sizeof(TCHAR));
    int numConverted = mbstowcs_s(&szi, pipeName, MAX_PIPE_SIZE, _pipeName, MAX_PIPE_SIZE);
    #else
    pipeName = _pipeName;
    #endif
    */

    //FFUNC_PRINTF(TEXT("PIPENAME IS %s"), pipeName);
    pipe = CreateFileA(_pipeName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (pipe == INVALID_HANDLE_VALUE) {
        FFUNC_ERROR("Create file FAILED");
        return -1;
    }

    FCGX_Init();
    if (!ffunc_init(ffunc_nmap_func, init_handler_name)) {
        exit(1);
    }

    char port_str[12];
    ffunc_worker_t *worker_t = malloc(sizeof(ffunc_worker_t));
    if (worker_t == NULL) {
        perror("nomem");
    }

    worker_t->accept_mutex = CreateMutex(NULL, FALSE, NULL); // this is lock for all threads
    int i;

    if (sock_port) {
        sprintf_s(port_str, sizeof port_str, ":%d", sock_port);
        if (backlog)
            worker_t->fcgi_func_socket = FCGX_OpenSocket(port_str, backlog);
        else
            worker_t->fcgi_func_socket = FCGX_OpenSocket(port_str, 50);
    } else if(sock_port_str) {
      if (backlog)
          worker_t->fcgi_func_socket = FCGX_OpenSocket(sock_port_str, backlog);
      else
          worker_t->fcgi_func_socket = FCGX_OpenSocket(sock_port_str, 50);
    }
    else {
        FFUNC_PRINTF(TEXT("%s\n"), TEXT("argument wrong"));
        exit(1);
    }

    if (worker_t->fcgi_func_socket < 0) {
        FFUNC_PRINTF(TEXT("%s\n"), TEXT("unable to open the socket"));
        exit(1);
    }

    /* Release of successfully initialized */
    DWORD numWritten;
    WriteFile(pipe, TEXT("DONE"), sizeof(TEXT("DONE")), &numWritten, NULL);
    CloseHandle(pipe);

    HANDLE  *pth_workers = calloc(max_thread, sizeof(HANDLE));
    for (i = 0; i < max_thread; i++) {
        unsigned thread_id;
        pth_workers[i] = (HANDLE)_beginthreadex(NULL, 0, ffunc_thread_worker, worker_t, 0, &thread_id);
    }
    for (i = 0; i < max_thread; i++) {
        WaitForSingleObject(pth_workers[i], INFINITE);
    }
    free(pth_workers);
    FFUNC_PRINTF(TEXT("%s\n"), TEXT("Exiting"));
    return EXIT_SUCCESS;
}

int
main(int argc, char *argv[]) {
    ffunc_config_t *conf;
    char **new_argv;
    unsigned int i = 1;
    int status;
    char* exec_name;
    char *sockport_str,
        *backlog_str,
        *max_thread_str,
        *max_read_buffer_str,
        *func_str, 
		*init_handler_name;

    if (((sockport_str = FFUNC_GETENV("_FFUNC_ENV_SOCK_PORT")) || (sockport_str = FFUNC_GETENV("_FFUNC_ENV_SOCK_PORT_STR"))) &&
        (backlog_str = FFUNC_GETENV("_FFUNC_ENV_BACKLOG")) &&
        (max_thread_str = FFUNC_GETENV("_FFUNC_ENV_MAX_THREAD")) &&
        (func_str = FFUNC_GETENV("_FFUNC_ENV_FUNC_str"))
        ) {
        conf = calloc(1, sizeof(ffunc_config_t));

        if(FFUNC_GETENV("_FFUNC_ENV_SOCK_PORT")) {
          conf->sock_port = atoi(sockport_str);
        } else if(FFUNC_GETENV("_FFUNC_ENV_SOCK_PORT_STR")) {
          conf->sock_port_str = sockport_str;
        }

		if ( (init_handler_name = FFUNC_GETENV("_FFUNC_ENV_INIT_HANDLER_NAME")) ) {
			conf->app_init_handler_name = init_handler_name;
		}

        conf->backlog = atoi(backlog_str);
        conf->max_thread = atoi(max_thread_str);
        if ((max_read_buffer_str = FFUNC_GETENV("_FFUNC_ENV_MAX_READ_BUF"))) {
            conf->max_read_buffer = (size_t)atol(max_read_buffer_str);
        }
        int func_count = 0, index = 0;
        char* p = func_str;
        while ((p = strstr(p, "\",\" "))) {
            func_count++;
            p += sizeof("\",\" ") - 1;
        }
        if (func_count > 0) {
            conf->ffunc_nmap_func = (char**)calloc(func_count + 1, sizeof(char*));
            p = func_str;
            while ((strstr(p, "\",\" "))) {
                conf->ffunc_nmap_func[index++] = p;
                p = strstr(p, "\",\" ");
                *p = (char)0; // char terminate for previos function
                p += sizeof("\",\" ") - 1;
            }
            conf->ffunc_nmap_func[index++] = NULL;
            /*for (index = 0; conf->ffunc_nmap_func[index]; index++) {
            printf("conf->ffunc_nmap_func %s\n", conf->ffunc_nmap_func[index]);
            }*/
        }
        else {
            FFUNC_ERROR("Function has no defined, [ conf->ffunc_nmap_func ] ");
            return -1;
        }
        return ffunc_hook(argv, conf, 1);
    }
    else if ((exec_name = FFUNC_GETENV(_FFUNC_MASTER_ENV))) {
        conf = calloc(1, sizeof(ffunc_config_t));
        conf->sock_port_str = NULL;
        conf->__exec_name = exec_name;
        ffunc_proc_name.data = argv[0];
        ffunc_proc_name.len = strlen(argv[0]);

        if ((status = ffunc_main(argc, argv, conf)) == 0) {
            return ffunc_hook(argv, conf, 0);
        }
        else {
            fprintf(stderr, "Error status %d, status return is not successful(0) \n", status);
            return status;
        }
    }
    else {
        FFUNC_SETENV(_FFUNC_MASTER_ENV, argv[0], 1);
        new_argv = calloc(argc + 1, sizeof(char*));
        size_t arg$0sz = strlen(argv[0]);
        new_argv[0] = calloc(arg$0sz + sizeof(_FFUNC_MASTER_), sizeof(char));
        memcpy(new_argv[0], argv[0], arg$0sz);
        memcpy(new_argv[0] + arg$0sz, _FFUNC_MASTER_, sizeof(_FFUNC_MASTER_) - 1);

        while (argv[i]) {
            new_argv[i] = argv[i++];
        }

        new_argv[i] = NULL;
        extern char **environ;
        FFUNC_EXECVE(_P_NOWAIT, argv[0], new_argv, environ);
    }
    return 0;
}
#endif
#endif

static void
ffunc_default_direct_consume(ffunc_session_t *sess) {
    ffunc_write_out(sess, "Content-Type: text/plain\r\n\r\n");
    ffunc_write_out(sess, "%s\n", "FN_HANDLER not found, ffunc_direct_consume not found");
    ffunc_write_out(sess, "%s\n", "Please provide your FN_HANDLER in your config file, else provide \"ffunc_direct_consume(ffunc_session_t *sess){}\" to customize your inputs");
    ffunc_write_out(sess, "%s\n", "For e.g nginx.conf fastcgi_param FN_HANDLER getProfile");
}

static int
ffunc_get_number_of_digit(long long number) {
    int count = 0;
    while (number != 0)
    {
        number /= 10;
        ++count;
    }
    return count;
}

static int
ffunc_strpos(const char *haystack, const char *needle)
{
    char *p = strstr((char*)haystack, needle);
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
            if ((pos = ffunc_strpos(qs, key)) < 0) return NULL;

            if (pos == 0 || qs[pos - 1] == '&') {
                qs = (char*)qs + pos + len;
                if (*qs++ == '=') {
                    char *src = qs,
                        *ret;
                    size_t sz = 0;
                    while (*qs && *qs++ != '&')sz++;

                    ret = (char *)_ffunc_alloc(&csession->pool, sz + 1);
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

void
ffunc_destroy_pool(ffunc_pool *p) {
    if (p) {
        if (p->prev) {
            ffunc_destroy_pool(p->prev);
        }
        free(p);
    }
}

size_t
ffunc_mem_left(ffunc_pool *p) {
    return (uintptr_t)p->end - (uintptr_t)p->next;
}

size_t
ffunc_mem_used(ffunc_pool *p) {
    return (uintptr_t)p->next - (uintptr_t)&p[1];
}

size_t
ffunc_blk_size(ffunc_pool *p) {
    return (uintptr_t)p->end - (uintptr_t)(void*)&p[1];
}

static void*
mem_align(size_t size) //alignment => 16
{
    void  *p;
#ifdef POSIX_MEMALIGN
    int    err;
    err = posix_memalign(&p, ALIGNMENT, size);
    if (err) {
        // FFUNC_PRINTF("posix_memalign(%uz, %uz) failed \n", ALIGNMENT, size);
        p = NULL;
    }
    //  FFUNC_PRINTF("posix_memalign: %p:%uz @%uz \n", p, size, ALIGNMENT);
#else
    // FFUNC_PRINTF("%s\n", "Using Malloc");
    p = malloc(size);

#endif
    return p;
}


ffunc_pool*
ffunc_create_pool(size_t size) {
    ffunc_pool * p = (ffunc_pool*)mem_align(size + sizeof(ffunc_pool));
    p->next = (void*)&p[1]; //p + sizeof(ffunc_pool)
    p->end = (void*)((uintptr_t)p->next + size);
    p->prev = NULL;
    return p;    // p->memblk = //mem_align( 16, DEFAULT_BLK_SZ);
}

static ffunc_pool*
ffunc_recreate_pool(ffunc_pool *curr_p, size_t new_size) {
    ffunc_pool *newp = NULL;
    if (curr_p) {
        newp = (ffunc_pool*)ffunc_create_pool(new_size);
        newp->prev = curr_p;
    }
    return newp;
}

/* 1 true, 0 false */
static int
ffunc_get_fit_pool(ffunc_pool **p, size_t fit_size) {
    ffunc_pool *curr_pool = *p, *prev_pool;
    while ((prev_pool = curr_pool->prev)) {
        if (ffunc_mem_left(prev_pool) >= fit_size) {
            *p = prev_pool;
            return 1;
        }
        curr_pool = curr_pool->prev;
    }
    return 0;
}

void *
_ffunc_alloc(ffunc_pool **p, size_t size) {
    ffunc_pool *curr_p = *p;
    if (ffunc_mem_left(curr_p) < size) {
        if (!(ffunc_get_fit_pool(&curr_p, size))) {
            size_t curr_blk_sz = ffunc_blk_size(curr_p);
            size_t new_size = curr_blk_sz * 2;

            while (new_size < size) {
                new_size *= 2;
            }

            *p = curr_p = ffunc_recreate_pool(curr_p, new_size);
        }
    }
    void *mem = (void*)curr_p->next;
    curr_p->next = (void*)((uintptr_t)curr_p->next + size); // alloc new memory
                                                            // memset(curr_p->next, 0, 1); // split for next

    return mem;
}

/* For Pool Testing purpose */
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
