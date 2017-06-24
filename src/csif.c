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
#include "csif.h"

// char* csif_nmap_func[];

csif_map *dyna_funcs = NULL;

typedef int (*h)(FCGX_Request *, csif_t*);

struct csif_handler {
    h handle;
};

static const long MAX_STDIN = 10240;
int no_debug(FILE *__restrict __stream, const char *__restrict __format, ...) {return 1;}

char *appname; //global apps deployment
static csif_LFHashTable thread_table;
static int total_workers, remain_workers;
void csif_init(char** csif_nmap_func);

void* Malloc_Function(size_t sz);
void Free_Function(void* v);

void* multi_thread_accept(void* sock_ptr);
// void *thread_admin(void* sock);
void *add_on_workers(void* sock);
int hook_socket(char* sock_port, int backlog, int workers, char** csif_nmap_func);
sigset_t* handle_request(FCGX_Request *request);
// int init_error_signal(f_singal_t *ferr_signals);
void* init_all_error_signal(void *v);
static struct sigaction sa;

void* Malloc_Function(size_t sz) {
    return malloc(sz);
}
void Free_Function(void* v) {
    // printf("%s\n", "FREE-------------- ");
    free(v);
}

void csif_init(char** csif_nmap_func) {
    csif_hash_set_alloc_fn(Malloc_Function, Free_Function);
    csif_buf_set_alloc_fn(Malloc_Function, Free_Function);


    void *usr_req_handle = NULL;
    char *error;
    usr_req_handle = dlopen(appname, RTLD_LAZY | RTLD_NOW);
    if (!usr_req_handle) {
        flog_err("%s\n", dlerror());
        flog_info("%s\n", "Have you included -rdynamic in your compiler option, for e.g gcc ... -rdynamic");
    }

    int func_count = 0;

    while (*(csif_nmap_func + func_count++));

    printf("total function = %d\n", --func_count); // Remove NULL
    dyna_funcs = csif_map_init(func_count, sizeof(struct csif_handler));

    for (int f = 0; f < func_count; f++) {
        // printf("%d\n", f);
        struct csif_handler *func = csif_map_assign(dyna_funcs, csif_nmap_func[f]);

        *(void **) (&func->handle) = dlsym(usr_req_handle, csif_nmap_func[f]);

        if ((error = dlerror()) != NULL) {
            flog_err("%s\n", error);
            flog_info("%s\n", "Have you included -rdynamic in your compiler option, for e.g gcc ... -rdynamic");
        }
    }

    // never close the library
    // if (dlclose(usr_req_handle) != 0) {
    //     flog_err("%s\n", "Unable to load feed module lib:");
    //     flog_err("%s\n", dlerror());
    // }
}

/**To prevent -std=99 issue**/
char *duplistr(const char *str) {
    int n = strlen(str) + 1;
    csif_t * csif = csif_get_t();
    char *dup = falloc(&csif->pool, n);
    memset(dup, 0, n);
    if (dup)
        memcpy(dup, str, n - 1);
    // dup[n - 1] = '\0';
    // please do not put flog_info here, it will recurve
    return dup;
}

int setup_logger(char* out_file_path) {
    FILE* writeFile;
    printf("%s\n", "initialize log file");
    if ((writeFile = fopen(out_file_path, "ab+")) == NULL) {
        printf("%s\n", "fopen source-file");
        return 0;
    }

    int fd = fileno(writeFile);

    // dup2(fd, STDIN_FILENO);
    // dup2(fd, STDOUT_FILENO);
    // dup2(fd, STDERR_FILENO);

    // fclose(writeFile);
    if (dup2(fd, STDERR_FILENO) < 0) return 0;


    // if (freopen( out_file_path, "ab+", FLOGGER_ ) == NULL) {
    //     flog_err("%s\n", "fopen source-file");
    //     return 0;
    // }
    return 1;
}

char* print_time(void) {
    char buff[20];
    time_t now = time(NULL);
    strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));
    return duplistr(buff);
}


sigset_t* handle_request(FCGX_Request *request) {
    sigset_t *status_mask = NULL;
    // cJSON_Hooks *hooks = malloc(sizeof(cJSON_Hooks));
    // hooks->malloc_fn = Malloc_Function;
    // hooks->free_fn = Free_Function;
    // fdebug("this pthread id is %lu\n", pthread_self());
    // //for cjson
    // cJSON_InitHooks(hooks);

    // flog_info("dummy value = %d\n", dummy++) ;
    // usleep(1000 * 1000 * 5); // 5 secs sleep
    // Dl_info info;
    char *value;
    // int (*handler)(FCGX_Request *, csif_t*);
    // static void *usr_req_handle = NULL;
    char **env = request->envp;

    csif_pool *p = create_pool(DEFAULT_BLK_SZ);

    // fhash_lock(thread_table);

    csif_t *_csif_ = malloc(sizeof(csif_t));

    csif_LF_put(&thread_table, pthread_self(), _csif_);

    // fhash_unlock(thread_table);
    _csif_->pool = p;
    _csif_->query_str = NULL;



    while (*(env) && strncmp(*env, "FN_HANDLER", 10) != 0) env++;
    value = (char*)(uintptr_t) * (env) + 11;

    if (value != NULL) {
        struct csif_handler *handler_ = (struct csif_handler*)csif_map_get(dyna_funcs, value);
        if (handler_) {
            if (strcmp(get_param("REQUEST_METHOD"), "GET") == 0) {
                char* query_string = duplistr(get_param("QUERY_STRING"));
                if (!is_empty(query_string))
                    _csif_->query_str = query_string;//parse_json_fr_args(query_string);
            }

            // trying to catch back to life from error
            if (/*sig*/setjmp(_csif_->jump_err_buff) == 999) {
                flog_err("%s\n", "Critical Error Found!!!!!!!!!!!!!!!!!!!!");
                // once catch , all signal are gone, you have to init again.
                // init_all_error_signal(); // init back again
                status_mask = ((csif_t *)csif_LF_get(&thread_table, pthread_self()))->curr_mask;
                // remain_workers--;
                goto SKIP_METHOD;
            }
            //
            // flog_info("dummy value = %d\n", 1);
            // (*fdyHandler->handle)(request, _csif_);

            handler_->handle(request, _csif_);
        } else {
            flog_err("%s\n", "Method not found!!!!!!!!!!!!!!!!!!!!");
SKIP_METHOD:
            csif_write_http_status(request, 500);
            write_out("Content-Type: text/plain\r\n\r\n");
            write_out("%s\n", "Method not found or memory corruption");
            write_out("%s\n", "check the log whether memory leak or method does not existed in your program");
        }
    } else {
        write_out("Content-Type: text/plain\r\n\r\n");
        write_out("%s\n", "FN_HANDLER not found");
        write_out("%s\n", "Please provide your FN_HANDLER in your config file");
        write_out("%s\n", "For e.g nginx.conf fastcgi_param FN_HANDLER getProfile");
    }
    // write_out("Content-Type: text/plain\r\n\r\n");
    // if ((value = get_param("REQUEST_METHOD")) != NULL) {
    //     write_out("%s\n", value);
    // }
    // if ((value = get_param("REQUEST_URI")) != NULL) {
    //     write_out("%s \n", value);
    // }
    // if ((value = get_param("QUERY_STRING")) != NULL) {
    //     write_out("?%s \n", value);
    // }
    // if ((value = get_param("SERVER_PROTOCOL")) != NULL) {
    //     write_out(" %s \n", value);
    // }
    // write_out("Done\n");
    // fhash_lock(thread_table);
    csif_t *f = csif_LF_pop(&thread_table, pthread_self());
    if (f) { //free the _csif_
        destroy_pool(f->pool);
        free(f);
        // fprintf(stdout, "%s\n", "successfully free _csif_:");
        // fprintf(stdout, "%s\n", "successfully freed");
        // so far all are freed.
        // fhash_unlock(thread_table);
    } else {
        destroy_pool(p);
        // fhash_unlock(thread_table);
    }
    return status_mask;
}

size_t slen(const char* str) {
    register const char *s;
    for (s = str; *s; ++s);
    return (s - str);
}

int csif_isspace(const char* s) {
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

int file_existed(const char* fname) {
    if ( access( fname, F_OK ) != -1 )
        return 1;
    else
        return 0;
}
// bug
// char *getBodyContent(FCGX_Request *request) {
//     char* clen = atoi(get_param("CONTENT_LENGTH"));
//     if (clen) {
//         int len = strtol(clen, &clen, 10);
//         char *buf = Malloc_Function((len + 1) * sizeof(char));

//         memset(buf, 0, len + 1);
//         while (FCGX_GetStr(buf, len, request->in) > 0); /*{
//         write_out("%s ", buf);
//     }*/
//         while (FCGX_GetChar(request->in) != -1);

//         return buf;
//     } else {
//         return NULL;
//     }
// }

void* csif_getParam(const char *key, char* query_str) {
    int len;
    char *qs = query_str;
    if (key && *key && qs && *qs) {
        len = strlen(key);
        do {
            if ((strncmp(key, qs, len) == 0) && (qs[len] == '=')) {
                char *src = qs = (char*)((uintptr_t)qs + len + 1),
                      *ret;
                size_t sz = 0;
                while (*qs && *qs++ != '&')sz++;

                csif_t * csif = csif_get_t();
                ret = falloc(&csif->pool, sz + 1);
                memset(ret, 0, sz + 1);
                return memcpy(ret, src, sz);
            }
            qs = (char*)((uintptr_t)qs + len + 1);
            while (*qs && *qs++ != '&');
            qs++;
        } while (*qs);
    }
    return NULL;
}

long csif_readContent(FCGX_Request *request, char** content) {
    char* clenstr = get_param("CONTENT_LENGTH");
    FCGX_Stream *in = request->in;
    long len;

    if (clenstr) {
        // long int strtol (const char* str, char** endptr, int base);
        // endptr -- Reference to an object of type char*, whose value is set by the function to the next character in str after the numerical value.
        // This parameter can also be a null pointer, in which case it is not used.
        len = strtol(clenstr, &clenstr, 10);
        /* Don't read more than the predefined limit  */
        if (len > MAX_STDIN) { len = MAX_STDIN; }

        csif_t * csif = csif_get_t();
        *content = falloc(&csif->pool, len + 1);
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

csif_t *csif_get_t(void) {
    return (csif_t *)csif_LF_get(&thread_table, pthread_self());
}

int init_socket(char* sock_port, int backlog, int workers, int forkable, int signalable, int debugmode, char* logfile, char* solib, char** csif_nmap_func) {

    if (!csif_nmap_func[0]) {
        printf("%s\n", "csif_nmap_func has no defined...");
        return 1;
    }

    if (debugmode) {
        csif_dbg = fprintf;
    }
    appname = solib;
    pid_t childPID;
    // FCGX_Init();
    // if ((logfile != NULL) && (logfile[0] == '\0'))setup_logger(logfile);
    if (logfile && logfile[0]) {
        setup_logger(logfile);
    }

    printf("%s\n", "Starting TCP Socket");
    printf("sock_port=%s, backlog=%d\n", sock_port, backlog);

    if (forkable) {
        childPID = fork();
        if (childPID >= 0) // fork was successful
        {
            if (childPID == 0) // child process
            {
                if (signalable) {
                    pthread_t pth;
                    pthread_create(&pth, NULL, init_all_error_signal, "Process Error Signal");
                }

                return hook_socket(sock_port, backlog, workers, csif_nmap_func);
            } else {}//Parent process
        } else {// fork failed
            printf("\n Fork failed..\n");
            return 1;
        }
    } else {
        if (signalable) {
            pthread_t pth;
            pthread_create(&pth, NULL, init_all_error_signal, "Process Error Signal");
        }

        return hook_socket(sock_port, backlog, workers, csif_nmap_func);
    }
    return 0;
}



int hook_socket(char* sock_port, int backlog, int workers, char** csif_nmap_func) {
    int sock = 0;
    FCGX_Init();
    csif_init(csif_nmap_func);

    remain_workers = 0;
    total_workers = workers;

    if (sock_port) {
        if (backlog)
            sock = FCGX_OpenSocket(sock_port, backlog);
        else
            sock = FCGX_OpenSocket(sock_port, 50);
    } else {
        printf("%s\n", "argument wrong");
        exit(1);
    }

    if (sock < 0) {
        printf("%s\n", "unable to open the socket" );
        exit(1);
    }

    printf("%d workers in process", workers);
    csif_LF_init(&thread_table, workers + 10); // give some space
    if (workers == 1) {
        FCGX_Request request;
        FCGX_InitRequest(&request, sock, 0);

        printf("Socket on hook %s", sock_port);

        while (FCGX_Accept_r(&request) >= 0) {
            handle_request(&request);
            FCGX_Finish_r(&request);
        }
    } else {
        for (;;) {
            // at least 5 workers running
            if (remain_workers < 5 ) {
                pthread_t add_workers_t;
                pthread_create(&add_workers_t, NULL, add_on_workers, &sock);
                remain_workers = remain_workers + total_workers;
            }

            // usleep(1000 * 1000 * 5); // 5 secs sleep
            sleep(5);
        }

    }

    printf("%s\n", "Exitingggg");
    return EXIT_SUCCESS;
}

void *multi_thread_accept(void* sock_ptr) {
    int rc;
    int* sock = (int*) sock_ptr;

    FCGX_Request request;


//     f_singal_t ferr_signals[] = {
//         { SIGABRT, "SIGABRT", "", error_handler },
// #ifdef SIGBUS
//         { SIGBUS, "SIGBUS", "", error_handler },
// #endif
//         { SIGFPE, "SIGFPE", "", error_handler },
//         { SIGILL, "SIGILL", "", error_handler },
//         { SIGIOT, "SIGIOT", "", error_handler },
//         { SIGSEGV, "SIGSEGV", "", error_handler},
//         { 0, NULL, "", NULL }
//     };

    if (FCGX_InitRequest(&request, *sock, 0) != 0) {
        printf("Can not init request\n");
        return NULL;
    }
    for (;;) {
        static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER; // this is lock for all threads

        // trying to accept new request
        pthread_mutex_lock(&accept_mutex);
        rc = FCGX_Accept_r(&request);
        pthread_mutex_unlock(&accept_mutex);

        if (rc < 0) {
            printf("%s\n", "Can not accept new request");
            break;
        }

        sigset_t *_mask_ = handle_request(&request);

        // server_name = FCGX_GetParam("SERVER_NAME", request.envp);

        // FCGX_PutS("Content-type: text/html\r\n", request.out);
        // FCGX_PutS("\r\n", request.out);
        // FCGX_PutS("<html>\r\n", request.out);
        // FCGX_PutS("<head>\r\n", request.out);
        // FCGX_PutS("<title>FastCGI Hello! (multi-threaded C, fcgiapp library)</title>\r\n", request.out);
        // FCGX_PutS("</head>\r\n", request.out);
        // FCGX_PutS("<body>\r\n", request.out);
        // FCGX_PutS("<h1>FastCGI Hello! (multi-threaded C, fcgiapp library)</h1>\r\n", request.out);
        // FCGX_PutS("<p>Request accepted from host <i>", request.out);
        // FCGX_PutS(server_name ? server_name : "?", request.out);
        // FCGX_PutS("</i></p>\r\n", request.out);
        // FCGX_PutS("</body>\r\n", request.out);
        // FCGX_PutS("</html>\r\n", request.out);
        FCGX_Finish_r(&request); // this will free all the fcgiparams memory and request
        /***If _mask_ is not NULL, means there is a error signal, need to suspend the thread completely **/
        if (_mask_) {
            // For detached threads, the system recycles its underlying resources automatically after the thread terminates.
            pthread_detach(pthread_self());
            remain_workers--; // release one worker
            printf("%s\n", "Detached");
            sigsuspend(_mask_);
            // pthread_exit(NULL);
        }
    }
    return NULL;
}

void *add_on_workers(void* sock) {
    int i;
    printf("add on workers = %d\n", total_workers);
    pthread_t pid[total_workers];
    for (i = 0; i < total_workers; i++) {
        pthread_create(&pid[i], NULL, multi_thread_accept, sock);
    }

    for (i = 0; i < total_workers; i++) {
        pthread_join(pid[i], NULL);
        printf("%s\n", "Detached from thread");
    }

    pthread_detach(pthread_self());
    // pthread_exit(NULL);
    return NULL;
}

// void *thread_admin(void* sock) {
//     while (1) {
//         // at least 5 workers running
//         if (remain_workers < 5 ) {
//             pthread_t add_workers_t;
//             pthread_create(&add_workers_t, NULL, add_on_workers, sock);
//             remain_workers = remain_workers + total_workers;
//         }

//         usleep(1000 * 1000 * 5); // 5 secs sleep
//     }

//     // pthread_exit(NULL);
//     pthread_detach(pthread_self());

// }

/***Error Signal Handler Scope***/
void error_handler(int signo);
void _backtrace(int fd, int signo,  int stack_size);


void _backtrace(int fd, int signo,  int stack_size) {
    // void *buffer = ngx_pcalloc(ngx_cycle->pool, sizeof(void *) * stack_size);
    void *buffer[stack_size];
    int size = 0;

    if (buffer != NULL) {
        // goto KillProcess;
        size = backtrace(buffer, stack_size);
        backtrace_symbols_fd(buffer, size, fd);
    }
    // size = backtrace(buffer, stack_size);
    // backtrace_symbols_fd(buffer, size, fd);

// KillProcess:
//     kill(getpid(), signo);
}

void error_handler(int signo)
{
    // f_singal_t *sig;
    // struct sigaction sa;

    // // for (sig = ferr_signals; sig->signo != 0; sig++) {
    // //     if (sig->signo == signo) {
    // //         break;
    // //     }
    // // }
    // memset(&sa, 0, sizeof(struct sigaction));
    // sa.sa_handler = error_handler;
    // sigemptyset(&sa.sa_mask);
    // if (sigaction(signo, &sa, NULL) == -1) {
    //     // flog_err("err sigaction(%s) failed", sig->signame);
    //     flog_err("%s\n", "err sigaction(?) failed");
    // }

    csif_t *_csif_ = csif_get_t();
    _csif_->curr_mask = &sa.sa_mask;

    int fd = fileno(FLOGGER_);
    fseek(FLOGGER_, 0, SEEK_END);
    _backtrace(fd, signo, 10);

//     switch (signo) {
//     case SIGABRT:
//     case SIGFPE:
//     case SIGILL:
//     // case SIGIOT:
//     case SIGSEGV:
// #ifdef SIGBUS
//     case SIGBUS:
// #endif
//         sigsuspend(&sa.sa_mask);
//         break;
//     default:
//         break;

//     }


    /*sig*/longjmp(_csif_->jump_err_buff, 999);
}

// int init_error_signal(f_singal_t *ferr_signals) {
//     f_singal_t *sig;
//     struct sigaction sa;
//     printf("%s\n", "initialize signals");
//     for (sig = ferr_signals; sig->signo != 0; sig++) {
//         memset(&sa, 0, sizeof(struct sigaction));
//         sa.sa_handler = sig->handler;
//         sigemptyset(&sa.sa_mask);
//         if (sigaction(sig->signo, &sa, NULL) == -1) {
//             printf("sigaction(%s) failed", sig->signame);
//             return 0;
//         }
//     }
//     return 1;
// }

void* init_all_error_signal(void *v) {
    printf("%s\n", "initialize signals");
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = error_handler;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
    sigaction(SIGIOT, &sa, NULL);
    sigaction(SIGSEGV, &sa, NULL);
#ifdef SIGBUS
    sigaction(SIGBUS, &sa, NULL);
#endif

    while (1) {
        sleep(3000);
    }

    pthread_detach(pthread_self());
    return NULL;
}

void csif_write_http_status(FCGX_Request * request, uint16_t code) {

    switch (code) {
    case 200:
        write_out("Status: 200 OK\r\n");
        break;
    case 204:
        write_out("Status: 204 No Content\r\n");
        break;

    case 500:
        write_out("Status: 500 Internal Server Error\r\n");
        break;

    default:
        write_out("Status: 200 OK\r\n");
        break;
    }
}

void csif_write_default_header(FCGX_Request * request) {
    write_out("Content-Type: application/x-www-form-urlencoded\r\n\r\n");
}

void csif_write_jsonp_header(FCGX_Request * request) {
    write_out("Content-Type: application/javascript\r\n\r\n");
}

void csif_write_json_header(FCGX_Request * request) {
    write_out("Content-Type: application/json\r\n\r\n");
}
