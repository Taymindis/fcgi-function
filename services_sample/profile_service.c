#include "ffunc.h"
// #include <ffunc/ffunc_mem_detect.h>

/*Compile*/
/*gcc profile_service.c -I../ ../ffunc.c -lfcgi -pthread -ldl -rdynamic*/
static int req_count = 0;
void getProfile(ffunc_session_t * session) {
	ffunc_write_out(session, "Status: 200 OK\r\n");
	ffunc_write_out(session, "Content-Type: text/plain\r\n\r\n");/* \r\n\r\n  means go to response message*/
	ffunc_write_out(session, "%s\n", "you are here");
	if (session->query_str) {
		char *out = ffunc_get_query_param(session, "userId", sizeof("userId") - 1);
		if (out)
			ffunc_write_out(session, "output= %s\n", out); //cjson undefined because only use it's own file

		out = ffunc_get_query_param(session, "userId2",  sizeof("userId2") - 1);
		if (out)
			ffunc_write_out(session, "output 2= %s\n", out); //cjson undefined because only use it's own file
	}
}

void postError(ffunc_session_t * session) {
	printf("%s\n", "you reach here with post Error test");
	ffunc_write_out(session, "Status: 500 Internal Server Error\r\n");
	ffunc_write_out(session, "Content-Type: text/plain\r\n\r\n");
	ffunc_write_out(session, "%s\n", "you hitting error");
}

void postProfile(ffunc_session_t * session) {
	// not need to free, csession handle it
	ffunc_str_t payload;
	if( ffunc_read_body(session, &payload) ) {
		printf("the sz is = %ld\n", payload.len);
		ffunc_write_out(session, "Status: 200 OK\r\n");
		ffunc_write_out(session, "Content-Type: application/x-www-form-urlencoded\r\n\r\n");
		ffunc_write_out(session, "Query String %s\n", session->query_str);
		ffunc_write_out(session, "Body %s\n", payload.data);
	}
}

int main (int argc, char *argv[]) {
	char* ffunc_nmap_func[] = {"getProfile", "postError", "postProfile", NULL};
	return ffunc_main (2005, 160, 64, ffunc_nmap_func, NULL);
}

// int memcheck(ffunc_session_t * csession) {
// 	// printf("%s\n", "you reach here");
// 	ffunc_write_out("Status: 200 OK\r\n");
// 	ffunc_write_out("Content-Type: text/plain\r\n\r\n"); \r\n\r\n  means go to response message
// 	ffunc_write_out("alloc= %d\n", get_total_malloc_count());
// 	ffunc_write_out("free= %d\n", get_total_free_count());
// 	ffunc_write_out("leak count= %d\n", get_total_malloc_count() - get_total_free_count());
// 	return 1;
// }
