#include <ffunc/ffunc_core.h>
// #include <ffunc/ffunc_mem_detect.h>

/*Compile*/
/*gcc ../services_sample/profile_service.c -lffunc -lfcgi -rdynamic -o simple_service*/
static int req_count = 0;
int getProfile(FCGX_Request *request, ffunc_session_t * csession) {
	// printf("%s-%d\n", "you reach here with get Request", __atomic_fetch_add(&req_count, 1 , __ATOMIC_SEQ_CST));
	ffunc_write_out("Status: 200 OK\r\n");
	ffunc_write_out("Content-Type: text/plain\r\n\r\n");/* \r\n\r\n  means go to response message*/
	ffunc_write_out("%s\n", "you are here");

	ffunc_session_t *session = ffunc_get_session();

	// printf("%s\n", session->json);
	// cJSON * ret;
	/* Generate a seg fault, for testing purpose */
	// static int ss = 0;
	// if (ss++ < 5000) *(int *)NULL = 0;
	if (session->query_str) {
		char *out = ffunc_getParam("userId", session->query_str);
		if (out)
			ffunc_write_out("output= %s\n", out); //cjson undefined because only use it's own file

		out = ffunc_getParam("userId2", session->query_str);
		if (out)
			ffunc_write_out("output 2= %s\n", out); //cjson undefined because only use it's own file
	}

	return 1;
}

int postError(FCGX_Request *request, ffunc_session_t * csession) {
	printf("%s\n", "you reach here with post Error test");
	ffunc_write_out("Status: 500 Internal Server Error\r\n");
	ffunc_write_out("Content-Type: text/plain\r\n\r\n");
	ffunc_write_out("%s\n", "you hitting error");

	return 1;
}


int postProfile(FCGX_Request *request, ffunc_session_t * csession) {
	// ffunc_buff *buffer = fbuf_init(get_param("QUERY_STRING"));
	// ffunc_write_out("Content-Type: text/plain\r\n\r\n");
	// ffunc_write_out("buffer size  = %d\n", buffer->size);
	// ffunc_write_out("buffer data  = %s\n", buffer->data);
	// fbuf_delete(buffer);
	// char *payload = getBodyContent(request);
	// ffunc_write_out("Content-Type: application/x-www-form-urlencoded\r\n\r\n");
	// ffunc_write_out("%s", "Data is ");
	// ffunc_write_out("%s\n", payload);

	// not need to free, csession handle it
	char *payload;
	long sz = ffunc_readContent(request, &payload);
	printf("the sz is = %ld", sz);
	ffunc_write_out("Status: 200 OK\r\n");
	ffunc_write_out("Content-Type: application/x-www-form-urlencoded\r\n\r\n");
	// ffunc_write_out("Content-Length: %d\r\n\r\n", sz);
	// ffunc_write_out("%s", "Data is ");
	// ffunc_write_out("%s\n", payload);
	// cJSON* thisObj = parse_json(payload);
	// if (sz && thisObj) {
	// 	cJSON *ret = (cJSON*)cjson_get_value(thisObj, "userId");
	// 	if (ret)
	// 		ffunc_write_out("%s\n", ret->valuestring);
	// 	ret = (cJSON*)cjson_get_value(thisObj, "timestamp");
	// 	if (ret)
	// 		ffunc_write_out("%s\n", ret->valuestring);
	// 	ret = (cJSON*)cjson_get_value(thisObj, "user_req");
	// 	if (ret)
	// 		ffunc_write_out("%s\n", ret->valuestring);

	// 	// ffunc_write_out("%s\n", cJSON_PrintUnformatted(thisObj));
	// 	cJSON_Delete(thisObj);
	// }

	printf("%s\n", payload);


	return 1;
}

int main (int argc, char *argv[]) {
	char* ffunc_nmap_func[] = {"getProfile", "postError", "postProfile", NULL};
	return ffunc_main (2005, 160, 10, ffunc_nmap_func, NULL);
}

// int memcheck(FCGX_Request *request, ffunc_session_t * csession) {
// 	// printf("%s\n", "you reach here");
// 	ffunc_write_out("Status: 200 OK\r\n");
// 	ffunc_write_out("Content-Type: text/plain\r\n\r\n"); \r\n\r\n  means go to response message
// 	ffunc_write_out("alloc= %d\n", get_total_malloc_count());
// 	ffunc_write_out("free= %d\n", get_total_free_count());
// 	ffunc_write_out("leak count= %d\n", get_total_malloc_count() - get_total_free_count());


// 	return 1;
// }
