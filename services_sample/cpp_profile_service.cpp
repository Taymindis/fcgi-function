#include <ffunc/ffunc_core.h>
#include <iostream>
#include <vector>
/*https://github.com/gabime/spdlog*/
#include <spdlog/spdlog.h>
/***
Using spdlog
g++ -std=c++11 -I/home/booking/clib/spdlog-0.17.0/include -DSPDLOG_FMT_PRINTF ../services_sample/cpp_profile_service.cpp -lffunc -lfcgi -rdynamic -o simple_service
**/
static std::shared_ptr<spdlog::logger> file_logger = 0;

class MyType
{
public:
	MyType() {
		file_logger->info("%s\n", "ASD");
	}
	~MyType() {
		file_logger->info("%s\n", "Value been freed");
	}
};


#ifdef __cplusplus
extern "C" {
#endif

void init_logger_in_instance(void);

void init_logger_in_instance() {
	fprintf(stderr, "%s\n", "start instance");
	fprintf(stderr, "%s\n", "init logging");
	auto rotating = std::make_shared<spdlog::sinks::rotating_file_sink_mt> ("simpledump.log", 1024 * 1024 * 5 /*5MB*/, 5);
	file_logger = spdlog::create_async("my_logger", rotating, 8192,
	                                   spdlog::async_overflow_policy::block_retry, nullptr, std::chrono::milliseconds{5000}/*flush interval*/, nullptr);

	// file_logger->set_pattern("%v"); // Just message
}

void getProfile(ffunc_session_t * session) {
	file_logger->info("%s\n", "you reach here with get Request");
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

void postError(ffunc_session_t * session) {
	file_logger->info("%s\n", "you reach here with post Error test");
	ffunc_write_out(session, "Status: 500 Internal Server Error\r\n");
	ffunc_write_out(session, "Content-Type: text/plain\r\n\r\n");
	ffunc_write_out(session, "%s\n", "you hitting error");
}


void postProfile(ffunc_session_t * session) {
	// not need to free, session handle it
	ffunc_str_t payload;
	if(ffunc_read_body(session, &payload) ) {
		file_logger->info("the sz is = %ld", payload.len);
		ffunc_write_out(session, "Status: 200 OK\r\n");
		ffunc_write_out(session, "Content-Type: application/x-www-form-urlencoded\r\n\r\n");
		ffunc_write_out(session, "Query String %s\n", session->query_str);
		ffunc_write_out(session, "Body %s\n", payload.data);
		file_logger->info("%s\n", payload.data);
	}

}

int main (int argc, char *argv[]) {
	char* ffunc_nmap_func[] = {"getProfile", "postError", "postProfile", NULL};
	return ffunc_main (2005, 160, 10, ffunc_nmap_func, init_logger_in_instance);
}


#ifdef __cplusplus
}
#endif
