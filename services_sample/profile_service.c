#include <restfcgi/feedy.h>
// #include <feedy/fdy_mem_detect.h>



char* fdy_nmap_func[] = {"getProfile", "postError","postProfile", NULL};


int getProfile(FCGX_Request *request, feedy_t * feedy) {
	flog_info("%s\n", "you reach here with get Request");
	write_out("Status: 200 OK\r\n");
	write_out("Content-Type: text/plain\r\n\r\n");/* \r\n\r\n  means go to response message*/
	write_out("%s\n", "you are here");

	feedy_t *sameFeedy= fdy_getFeedy(); 

	// printf("%s\n", sameFeedy->json);
	// cJSON * ret;
	/* Generate a seg fault, for testing purpose */
	// static int ss = 0;	
	// if (ss++ < 8) *(int *)NULL = 0;

	if (sameFeedy->query_str) {
		char *out = fdy_getParam("userId", sameFeedy->query_str);
		if (out)
			write_out("output= %s\n", out); //cjson undefined because only use it's own file
	}


	return 1;
}

int postError(FCGX_Request *request, feedy_t * feedy) {
	flog_info("%s\n", "you reach here with post Error test");
	write_out("Status: 500 Internal Server Error\r\n");
	write_out("Content-Type: text/plain\r\n\r\n");	
	write_out("%s\n", "you hitting error");

	return 1;
}


int postProfile(FCGX_Request *request, feedy_t * feedy) {
	// fdy_buff *buffer = fbuf_init(get_param("QUERY_STRING"));
	// write_out("Content-Type: text/plain\r\n\r\n");
	// write_out("buffer size  = %d\n", buffer->size);
	// write_out("buffer data  = %s\n", buffer->data);
	// fbuf_delete(buffer);
	// char *payload = getBodyContent(request);
	// write_out("Content-Type: application/x-www-form-urlencoded\r\n\r\n");
	// write_out("%s", "Data is ");
	// write_out("%s\n", payload);

	// not need to free, feedy handle it
	char *payload;
	long sz = fdy_readContent(request, &payload);
	flog_info("the sz is = %ld", sz);
	write_out("Status: 200 OK\r\n");
	write_out("Content-Type: application/x-www-form-urlencoded\r\n\r\n");
	// write_out("Content-Length: %d\r\n\r\n", sz);
	// write_out("%s", "Data is ");
	// write_out("%s\n", payload);
	cJSON* thisObj = parse_json(payload);
	if (sz && thisObj) {
		cJSON *ret = (cJSON*)cjson_get_value(thisObj, "userId");
		if (ret)
			write_out("%s\n", ret->valuestring);
		ret = (cJSON*)cjson_get_value(thisObj, "timestamp");
		if (ret)
			write_out("%s\n", ret->valuestring);
		ret = (cJSON*)cjson_get_value(thisObj, "user_req");
		if (ret)
			write_out("%s\n", ret->valuestring);

		// write_out("%s\n", cJSON_PrintUnformatted(thisObj));
		cJSON_Delete(thisObj);
	}

	flog_info("%s\n", payload);


	return 1;
}



// int memcheck(FCGX_Request *request, feedy_t * feedy) {
// 	// flog_info("%s\n", "you reach here");
// 	write_out("Status: 200 OK\r\n");
// 	write_out("Content-Type: text/plain\r\n\r\n"); \r\n\r\n  means go to response message
// 	write_out("alloc= %d\n", get_total_malloc_count());
// 	write_out("free= %d\n", get_total_free_count());
// 	write_out("leak count= %d\n", get_total_malloc_count() - get_total_free_count());


// 	return 1;
// }
