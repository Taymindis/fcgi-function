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

## How to Install

### Method 1. Just Include ffunc.h and ffunc.c in your project, simplest way
#### Build sample 
> gcc profile_service.c ffunc.c ffunc.h -lfcgi -pthread -ldl -rdynamic
> g++ profile_service.cpp ffunc.c ffunc.h -lfcgi -pthread -ldl -rdynamic
> g++ -std=c++11 -I/home/xxx/cpplib/spdlog-0.17.0/include -DSPDLOG_FMT_PRINTF service_with_spdlog.cpp ffunc.c ffunc.h -lfcgi -pthread -ldl -rdynamic

### Method 2. install as a library ( Linux/Darwin installation, for windows, use method 1)
#### 1. Go to root directory
#### 2. type 
> mkdir build (if not existed)

> cd build

> cmake ..

> make

> sudo make install


#### 3. the result will be
Install the project...

-- Install configuration: ""

-- Installing: /usr/local/lib/libffunc.dylib

-- Installing: /usr/local/lib/libffunc.a

-- Installing: /usr/local/include/ffunc/ffunc.h

#### 4. build a simple program by execute 

> gcc ../services_sample/profile_service.c -lffunc -lfcgi -rdynamic -o simple_service

#### For C++
> g++ -std=c++11 ../services_sample/cpp_profile_service.cpp -lffunc -lfcgi -latomic -rdynamic -o simple_service

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

### 6. simple start a service by execute 

> ./simple_service

### 6.1 Running on background by using nohup(preferable) or forking option
> nohup bash -c "./simple_service" >/dev/null 2>&1 &

### 6.2 Running on valgrind (performance will impact)
> valgrind --leak-check=full --show-leak-kinds=all --trace-children=yes ./simple_service

#### 6.2.1 valgrind on background (recommended for UAT purpose)
> nohup bash -c "valgrind --leak-check=full --show-leak-kinds=all --trace-children=yes --log-file=/home/user1/valgrind_check.log ./simple_service" >/dev/null 2>&1 &

### 7. Edit the nginx.conf in your nginx config folder by append in your server block:-

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

***You will see the FN_HANDLER is function name mapping with the function inside simple_service code, the fastcgi port 2005 is the service you start with(please look at step 10 for more details.***


### 8. start the nginx server

### 9.  Using apache benchmark for get request load test

> ab -c 100 -n 10000 http://127.0.0.1:80/getProfile


#### For post request load test

> ab -p "payload.txt" -T application/json -c 100 -n 10000 http://127.0.0.1:80/postProfile

*the payload.txt is inside the root directory*


### 10. shutdown the background instance 

##### kill the process by using `kill -2 <pid>`, *please do not use -9 as it will direct kill the process, unless -2 is not working
##### for valgrind log, you will get the summary report after `kill -2 <pid>`


## To uninstall.
### 1. Go to root_directory/build folder -- make sure build content is still existed.
### 2. type "sudo make uninstall" 
Then result

-- Uninstalling: /usr/local/lib/libffunc.dylib

-- Uninstalling: /usr/local/lib/libffunc.a

-- Uninstalling: /usr/local/include/ffunc.h

# Logging Recommendation
Due to built in logging mechanism will slow down the process speed, suggested to use third party logging mechanism for your application layer such as:-

[C++ spdlog](https://github.com/gabime/spdlog)

[C++ g3log](https://github.com/KjellKod/g3log)

[C mini-async-log](https://github.com/RafaGago/mini-async-log)


## Please contact minikawoon2017@gmail.com for More End to End tier project examples.
