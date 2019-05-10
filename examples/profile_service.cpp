#include "ffunc.h"
#include <iostream>
#include <vector>
/***
g++ profile_service.cpp -I../ ../ffunc.c -lfcgi -pthread -ldl -rdynamic
**/

class MyType
{
public:
	MyType() {
		printf("%s\n", "ASD");
	}
	~MyType() {
		printf("%s\n", "Value been freed");
	}
};


#ifdef __cplusplus
extern "C" {
#endif

void init_logger_in_instance(void);

void init_logger_in_instance() {
	fprintf(stderr, "%s\n", "start instance");
	fprintf(stderr, "%s\n", "init logging");
}

FFUNC getProfile(ffunc_session_t * session) {
	ffunc_write_out(session, "Status: 200 OK\r\n");
	ffunc_write_out(session, "Content-Type: text/plain\r\n\r\n");/* \r\n\r\n  means go to response message*/
	ffunc_write_out(session, "%s\n", "you are here");

	MyType me;
	std::vector<int> a;
	a.push_back(1);

	if (session->query_str) {
		char *out = (char*) ffunc_get_query_param(session, "userId", sizeof("userId")- 1);
		if (out)
			ffunc_write_out(session, "output= %s\n", out); //cjson undefined because only use it's own file
	}
}

FFUNC postError(ffunc_session_t * session) {
	ffunc_write_out(session, "Status: 500 Internal Server Error\r\n");
	ffunc_write_out(session, "Content-Type: text/plain\r\n\r\n");
	ffunc_write_out(session, "%s\n", "you hitting error");
}


FFUNC postProfile(ffunc_session_t * session) {
	// not need to free, session handle it
	ffunc_str_t payload;
	if(ffunc_read_body(session, &payload) ) {
		ffunc_write_out(session, "Status: 200 OK\r\n");
		ffunc_write_out(session, "Content-Type: application/x-www-form-urlencoded\r\n\r\n");
		ffunc_write_out(session, "Query String %s\n", session->query_str?session->query_str:"");
		ffunc_write_out(session, "Body %s\n", payload.data);
	}

}


int ffunc_main(int argc, char *argv[], ffunc_config_t *ffunc_conf) {
	ffunc_conf->sock_port = 2005;
	//memcpy(ffunc_conf->sock_port_str, "192.168.44.35:2005", sizeof("192.168.44.35:2005") - 1);
	ffunc_conf->backlog = 160;
	ffunc_conf->max_thread = 64;
	ffunc_parse_function(ffunc_conf, "getProfile", "postError", "postProfile");
	return 0;
}


#ifdef __cplusplus
}
#endif
