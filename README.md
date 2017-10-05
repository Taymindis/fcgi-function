# ngx-c-handler
ngx-c-handler is a C/CPP service interface, It is a Service Interface build on top of FastCGI and Nginx which provided function handler support, it also provided useful collection tools to enhance the facilities. 

## How it works
![Image of simpleflow](/images/simple-flow.png)


## Prerequisition Installed
Nginx -- https://www.nginx.com/

fcgi library -- https://github.com/FastCGI-Archives/FastCGI.com

cmake and make

### Supported OS: LINUX, MAC OSX

## Step of installation
### 1. Go to root directory
### 2. type 
> mkdir build (if not existed)

> cd build

> cmake ..

> make

> sudo make install


### 3. the result will be
Install the project...

-- Install configuration: ""

-- Installing: /usr/local/lib/libffunc.dylib

-- Installing: /usr/local/lib/libffunc.a

-- Installing: /usr/local/include/ffunc/ffunc_core.h

-- Installing: /usr/local/include/ffunc/ffunc_json.h

-- Installing: /usr/local/include/ffunc/ffunc_hash.h

-- Installing: /usr/local/include/ffunc/ffunc_LFHashTable.h

-- Installing: /usr/local/include/ffunc/ffunc_map.h

-- Installing: /usr/local/include/ffunc/ffunc_buf.h

-- Installing: /usr/local/include/ffunc/ffunc_pool.h


### 4. build a simple program by execute 

> gcc ../services_sample/profile_service.c -lffunc -lfcgi -rdynamic -o simple_service

#### For C++
> g++ -std=c++11 ../services_sample/cpp_profile_service.cpp -lffunc -lfcgi -rdynamic -o simple_service

### 5. when you type 

> ./simple_service

it will result:-
Available options:

	-a	the ip address which binding to
	
	-p	port number to specified, not for -s
	
	-s	unix domain socket path to generate, not for -p
	
	-q	number of socket backlog
	
	-w	number of worker process
	
	-l	log file path
	
	-e	signal handling
	
	-f	Fork Daemon process
	
	-d	Run on debug Mode
	
	-o	Dynamic Link shared object file
	
	-h	display usage
	
	-v	display version
	

### 6. simple start a service by execute 

> ./simple_service -p2005 -q200 -w200 -d -e

### 6.1 Running on background by using nohup(preferable) or forking option
> nohup bash -c "./simple_service -p2005 -w100 -q100 -d -e -l/home/user1/apps.log" >/dev/null 2>&1 &

> ./simple_service -p2005 -w100 -q100 -d -e -l/home/user1/apps.log -f

### 6.2 Running on valgrind (performance will impact)
> valgrind --leak-check=full --show-leak-kinds=all ./simple_service -p2005 -w200 -q200 -d -e

> valgrind --leak-check=full --show-leak-kinds=all ./simple_service -p2005 -w200 -q200 -d -e -l/home/user1/apps.log

#### 6.2.1 valgrind on background (recommended for UAT purpose)
> nohup bash -c "valgrind --leak-check=full --log-file=/home/user1/valgrind_check.log ./simple_service -p2005 -w200 -q200 -d -e -l/home/user1/apps.log" >/dev/null 2>&1 &

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

-- Uninstalling /usr/local/lib/libffunc.dylib

-- Uninstalling /usr/local/lib/libffunc.a

-- Uninstalling /usr/local/include/ffunc/ffunc_core.h

-- Uninstalling /usr/local/include/ffunc/ffunc_json.h

-- Uninstalling /usr/local/include/ffunc/ffunc_hash.h

-- Uninstalling /usr/local/include/ffunc/ffunc_LFHashTable.h

-- Uninstalling /usr/local/include/ffunc/ffunc_map.h

-- Uninstalling /usr/local/include/ffunc/ffunc_buf.h

-- Uninstalling /usr/local/include/ffunc/ffunc_pool.h

