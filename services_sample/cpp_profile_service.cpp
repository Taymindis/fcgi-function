#include <c_handler/ch_core.h>
// #include <c_handler/ch_mem_detect.h>
#include <iostream>
#include <vector>



class MyType
{
public:
	MyType() {
		flog_info("%s\n", "ASD");
	}
	~MyType(){
		flog_info("%s\n", "Value been freed");
	}
};




#ifdef __cplusplus
extern "C" {
#endif

int getProfile(FCGX_Request *request, ch_session_t * csession) {
	flog_info("%s\n", "you reach here with get Request");
	ch_write_out("Status: 200 OK\r\n");
	ch_write_out("Content-Type: text/plain\r\n\r\n");/* \r\n\r\n  means go to response message*/
	ch_write_out("%s\n", "you are here");

	ch_session_t *session = ch_get_session();

	MyType me;
	std::vector<int> a;
	a.push_back(1);

	// printf("%s\n", session->json);
	// cJSON * ret;
	/* Generate a seg fault, for testing purpose */
	// static int ss = 0;
	// if (ss++ < 8) *(int *)NULL = 0;

	if (session->query_str) {
		char *out = (char*) ch_getParam("userId", session->query_str);
		if (out)
			ch_write_out("output= %s\n", out); //cjson undefined because only use it's own file
	}

	return 1;
}

int postError(FCGX_Request *request, ch_session_t * csession) {
	flog_info("%s\n", "you reach here with post Error test");
	ch_write_out("Status: 500 Internal Server Error\r\n");
	ch_write_out("Content-Type: text/plain\r\n\r\n");
	ch_write_out("%s\n", "you hitting error");

	return 1;
}


int postProfile(FCGX_Request *request, ch_session_t * csession) {

	// not need to free, csession handle it
	char *payload;
	long sz = ch_readContent(request, &payload);
	flog_info("the sz is = %ld", sz);
	ch_write_out("Status: 200 OK\r\n");
	ch_write_out("Content-Type: application/x-www-form-urlencoded\r\n\r\n");
	// ch_write_out("Content-Length: %d\r\n\r\n", sz);
	// ch_write_out("%s", "Data is ");
	// ch_write_out("%s\n", payload);
	// cJSON* thisObj = parse_json(payload);
	// if (sz && thisObj) {
	// 	cJSON *ret = (cJSON*)cjson_get_value(thisObj, "userId");
	// 	if (ret)
	// 		ch_write_out("%s\n", ret->valuestring);
	// 	ret = (cJSON*)cjson_get_value(thisObj, "timestamp");
	// 	if (ret)
	// 		ch_write_out("%s\n", ret->valuestring);
	// 	ret = (cJSON*)cjson_get_value(thisObj, "user_req");
	// 	if (ret)
	// 		ch_write_out("%s\n", ret->valuestring);

	// 	// ch_write_out("%s\n", cJSON_PrintUnformatted(thisObj));
	// 	cJSON_Delete(thisObj);
	// }

	flog_info("%s\n", payload);


	return 1;
}

int main (int argc, char *argv[]) {
	char* ch_nmap_func[] = {"getProfile", "postError", "postProfile", NULL};
	ch_main (argc, argv, ch_nmap_func);
}


#ifdef __cplusplus
}
#endif





// int memcheck(FCGX_Request *request, ch_session_t * csession) {
// 	// flog_info("%s\n", "you reach here");
// 	ch_write_out("Status: 200 OK\r\n");
// 	ch_write_out("Content-Type: text/plain\r\n\r\n"); \r\n\r\n  means go to response message
// 	ch_write_out("alloc= %d\n", get_total_malloc_count());
// 	ch_write_out("free= %d\n", get_total_free_count());
// 	ch_write_out("leak count= %d\n", get_total_malloc_count() - get_total_free_count());


// 	return 1;
// }
