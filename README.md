# feedyC
feedyC is build by C Language. It is a Service build on top of FastCGI which provided function handler support, it also provided useful collection tools to enhance the facilities. 

## Prerequisition Installed
Nginx -- https://www.nginx.com/

cJSON libraries -- https://github.com/DaveGamble/cJSON

fcgi library -- https://github.com/FastCGI-Archives/FastCGI.com

cmake and make

### Supported OS: LINUX

## Step of installation
### 1. Go to root directory
### 2. type 

> mkdir build

 if not existed
### 3. type 

> cd build

### 4. type 

> cmake ..

### 5. type 

> make

### 6. type

> sudo make install

### 7. the result will be
Install the project...

-- Install configuration: ""

-- Installing: /usr/local/lib/libfeedy.so

-- Installing: /usr/local/lib/libfeedy.a

-- Installing: /usr/local/include/restfcgi/feedy.h

-- Installing: /usr/local/include/restfcgi/fdy_json.h

-- Installing: /usr/local/include/restfcgi/fdy_hash.h

-- Installing: /usr/local/include/restfcgi/fdy_LFHashTable.h

-- Installing: /usr/local/include/restfcgi/fdy_array.h

-- Installing: /usr/local/include/restfcgi/fdy_sarray.h

-- Installing: /usr/local/include/restfcgi/fdy_buf.h

-- Installing: /usr/local/include/restfcgi/fdy_pool.h


### 8. build a simple program by execute 

> gcc ../services_sample/profile_service.c -lfeedy -lcjson -lfcgi
> -rdynamic -o simple_service


### 9. when you type 

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
	

### 10. simple start a service by execute 

> ./simple_service -p2005 -q200 -w200 -d


### 11. Edit the nginx.conf in your nginx config folder by append in your server block:-

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


### 12. start the nginx server

### 13.  Using apache benchmark for get request load test

> ab -c 100 -n 10000 http://127.0.0.1:80/getProfile


#### For post request load test

> ab -p "payload.txt" -T application/json -c 100 -n 10000
> http://127.0.0.1:80/postProfile

*the payload.txt is inside the root directory*







## To uninstall.
### 1. Go to root_directory/build folder -- make sure build content is still existed.
### 2. type "sudo make uninstall" 
Then result

-- Uninstalling /usr/local/lib/libfeedy.so

-- Uninstalling /usr/local/lib/libfeedy.a

-- Uninstalling /usr/local/include/restfcgi/feedy.h

-- Uninstalling /usr/local/include/restfcgi/fdy_json.h

-- Uninstalling /usr/local/include/restfcgi/fdy_hash.h

-- Uninstalling /usr/local/include/restfcgi/fdy_LFHashTable.h

-- Uninstalling /usr/local/include/restfcgi/fdy_array.h

-- Uninstalling /usr/local/include/restfcgi/fdy_sarray.h

-- Uninstalling /usr/local/include/restfcgi/fdy_buf.h

-- Uninstalling /usr/local/include/restfcgi/fdy_pool.h
