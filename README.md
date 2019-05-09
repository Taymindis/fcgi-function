# fcgi-function
fcgi Function is a C/CPP language based interface, which build on top of FastCGI with built in functional handler support, it also provided useful collection tools to enhance the facilities. It can be integrated with Nginx Fastcgi module

## How it works
![Image of architecture](/images/fcgi_func_architecture.png)


## Prerequisition Installed
Nginx -- https://www.nginx.com/

fcgi library -- https://github.com/FastCGI-Archives/FastCGI.com

cmake and make

### Supported OS: LINUX, MAC OSX, and Windows

## Clone the Repository Recursively
git clone --recursive https://github.com/Taymindis/fcgi-function.git

## How to Build

### Just Include ffunc.h and ffunc.c in your project, simplest way ( applicable to Linux, Darwin and Windows platforms )
#### Build sample 
> gcc profile_service.c ffunc.c ffunc.h -lfcgi -pthread -ldl -rdynamic

> g++ profile_service.cpp ffunc.c ffunc.h -lfcgi -pthread -ldl -rdynamic

> g++ -std=c++11 -I/home/xxx/cpplib/spdlog-0.17.0/include -DSPDLOG_FMT_PRINTF service_with_spdlog.cpp ffunc.c ffunc.h -lfcgi -pthread -ldl -rdynamic

### when you type 

> ./simple_service

it will result:-

```bash

Service starting
sock_port=2005, backlog=16
total function = 3
64 threads in process
Socket on hook 2005

```


Running on background by setting `ffunc_conf->daemon = 1 `, see example for mode details
```c
int ffunc_main(int argc, char *argv[], ffunc_config_t *ffunc_conf) {
  ffunc_conf->sock_port = 2005;
  ffunc_conf->backlog = 160;
  ffunc_conf->max_thread = 64;
  ffunc_conf->daemon = 1; // run as daemon
  ffunc_parse_function(ffunc_conf, "getProfile", "postError", "postProfile");
  return 0;
}
```

### Running on valgrind (performance will impact)
> valgrind --leak-check=full --show-leak-kinds=all --trace-children=yes ./simple_service

### Edit the nginx.conf in your nginx config folder by append in your server block:-

	location /getProfile {
      add_header Allow "GET, POST, HEAD" always;
      if ( $request_method !~ ^(GET|HEAD)$ ) {
        return 405;
      }
      include /etc/nginx/fastcgi_params;
      fastcgi_param FN_HANDLER getProfile;
      fastcgi_pass 127.0.0.1:2005;
    }

    location /postProfile {
      add_header Allow "GET, POST, HEAD" always;
      if ( $request_method !~ ^(POST)$ ) {
        return 405;
      }
       include /etc/nginx/fastcgi_params;
      fastcgi_param FN_HANDLER postProfile;
      fastcgi_pass 127.0.0.1:2005;
    }

### Edit the httpd.conf in your httpd config folder by append in your block 

```httpd
<Location "/getProfile">
    # Timeout values are in seconds
    ProxyPass "fcgi://127.0.0.1:2005" connectiontimeout=300 timeout=300
    ProxyPassReverse "fcgi://127.0.0.1:2005"
    SetEnv  FN_HANDLER "getProfile"
</Location>

```

***You will see the FN_HANDLER is function name mapping with the function inside simple_service code, the fastcgi port 2005 is the service you start with(please look at step 10 for more details.***

### start the nginx/httpd server 

### Using apache benchmark for get request load test

> ab -c 100 -n 10000 http://127.0.0.1:80/getProfile


#### For post request load test

> ab -p "payload.txt" -T application/json -c 100 -n 10000 http://127.0.0.1:80/postProfile

*the payload.txt is inside the root directory*


### shutdown the background instance 

##### kill the process by using `kill -2 <pid>`, *please do not use -9 as it will direct kill the process, unless -2 is not working*
#### for valgrind log, you will get the summary report after `kill -2 <pid>` 


### Example of how to build a docker image 

```bash
cd DockerExample
docker build -t nginx_ffunc -f Dockerfile_ngx_ffunc .
docker run -d -p 80:80 --name testffunc nginx_ffunc
curl "http://127.0.0.1/getProfile"
```


### Logging Recommendation
Due to built in logging mechanism will slow down the process speed, suggested to use third party logging mechanism for your application layer such as:-

[C++ spdlog](https://github.com/gabime/spdlog)

[C++ g3log](https://github.com/KjellKod/g3log)

[C mini-async-log](https://github.com/RafaGago/mini-async-log)


## Please contact minikawoon2017@gmail.com for More End to End tier project examples.
