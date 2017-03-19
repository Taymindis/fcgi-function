#ifndef HEADER_FJSON
#define HEADER_FJSON

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <cjson/cJSON.h>


cJSON *parse_json_fr_args(char *payload);
cJSON *parse_json(char* content);
int req_param_separator_(char* f_payload, void* obj, int (*callback)(void* obj, char* key, char* val));
// cJSON *parse_json_fr_file(char* filename);
void* cjson_get_value(cJSON *item, const char* key);
void* cjson_get_index(cJSON *item, int i);
void* create_json_obj(void);
void* create_json_arr(void);


#ifdef __cplusplus
}
#endif

#endif