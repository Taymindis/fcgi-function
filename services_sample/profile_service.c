#include <csif/csif.h>
// #include <csif/csif_mem_detect.h>

int getProfile(FCGX_Request *request, csif_t * csif) {
	flog_info("%s\n", "you reach here with get Request");
	csif_write_out("Status: 200 OK\r\n");
	csif_write_out("Content-Type: text/plain\r\n\r\n");/* \r\n\r\n  means go to response message*/
	csif_write_out("%s\n", "you are here");

	csif_t *session= csif_read_t(); 

	// printf("%s\n", session->json);
	// cJSON * ret;
	/* Generate a seg fault, for testing purpose */
	// static int ss = 0;	
	// if (ss++ < 8) *(int *)NULL = 0;

	if (session->query_str) {
		char *out = csif_getParam("userId", session->query_str);
		if (out)
			csif_write_out("output= %s\n", out); //cjson undefined because only use it's own file
	}

	free(session);

	return 1;
}

int postError(FCGX_Request *request, csif_t * csif) {
	flog_info("%s\n", "you reach here with post Error test");
	csif_write_out("Status: 500 Internal Server Error\r\n");
	csif_write_out("Content-Type: text/plain\r\n\r\n");	
	csif_write_out("%s\n", "you hitting error");

	return 1;
}


int postProfile(FCGX_Request *request, csif_t * csif) {
	// csif_buff *buffer = fbuf_init(get_param("QUERY_STRING"));
	// csif_write_out("Content-Type: text/plain\r\n\r\n");
	// csif_write_out("buffer size  = %d\n", buffer->size);
	// csif_write_out("buffer data  = %s\n", buffer->data);
	// fbuf_delete(buffer);
	// char *payload = getBodyContent(request);
	// csif_write_out("Content-Type: application/x-www-form-urlencoded\r\n\r\n");
	// csif_write_out("%s", "Data is ");
	// csif_write_out("%s\n", payload);

	// not need to free, csif handle it
	char *payload;
	long sz = csif_readContent(request, &payload);
	flog_info("the sz is = %ld", sz);
	csif_write_out("Status: 200 OK\r\n");
	csif_write_out("Content-Type: application/x-www-form-urlencoded\r\n\r\n");
	// csif_write_out("Content-Length: %d\r\n\r\n", sz);
	// csif_write_out("%s", "Data is ");
	// csif_write_out("%s\n", payload);
	// cJSON* thisObj = parse_json(payload);
	// if (sz && thisObj) {
	// 	cJSON *ret = (cJSON*)cjson_get_value(thisObj, "userId");
	// 	if (ret)
	// 		csif_write_out("%s\n", ret->valuestring);
	// 	ret = (cJSON*)cjson_get_value(thisObj, "timestamp");
	// 	if (ret)
	// 		csif_write_out("%s\n", ret->valuestring);
	// 	ret = (cJSON*)cjson_get_value(thisObj, "user_req");
	// 	if (ret)
	// 		csif_write_out("%s\n", ret->valuestring);

	// 	// csif_write_out("%s\n", cJSON_PrintUnformatted(thisObj));
	// 	cJSON_Delete(thisObj);
	// }

	flog_info("%s\n", payload);


	return 1;
}

int main (int argc, char *argv[]) {
char* csif_nmap_func[] = {"getProfile", "postError","postProfile", NULL};
csif_main (argc, argv, csif_nmap_func);
}



// int memcheck(FCGX_Request *request, csif_t * csif) {
// 	// flog_info("%s\n", "you reach here");
// 	csif_write_out("Status: 200 OK\r\n");
// 	csif_write_out("Content-Type: text/plain\r\n\r\n"); \r\n\r\n  means go to response message
// 	csif_write_out("alloc= %d\n", get_total_malloc_count());
// 	csif_write_out("free= %d\n", get_total_free_count());
// 	csif_write_out("leak count= %d\n", get_total_malloc_count() - get_total_free_count());


// 	return 1;
// }
