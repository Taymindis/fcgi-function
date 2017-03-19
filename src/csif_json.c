
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include "csif.h"
#include "csif_json.h"


int parseCJson(char *text, cJSON **json_ptr);
// cJSON* parseCJsonFromFile(char *filename, cJSON **json_ptr);
int cjson_parse_prm_cb(void* item, char* key, char* val);
cJSON *_cjson_parse_param_(char *f_payload);
void* csif_create_cjson_obj(void);
void* csif_create_cjson_arr(void);

/*incubating*/
/****JSON COMMON*/
cJSON *parse_json_fr_args(char *payload) {
    cJSON * ret = NULL;
    if (payload  && payload[0]) {
        fdebug("GET Request args is %s", payload);
        fdebug("%s\n", "do cJSON Parsing" );
        ret = (cJSON*) _cjson_parse_param_((char*)payload);
        fdebug("%s\n", "Done Parsing");
        // fdebug("feed_json print is %s", cJSON_PrintUnformatted(ret));
    }
    return ret;
}

cJSON *parse_json(char* content) {
    cJSON *json_ = NULL;
    if (content && *content) {
        fdebug("%s\n", "do cJSON Parsing" );
        parseCJson(content, &json_);
        // fdebug("cjson printing: %s\n", cJSON_Print(json_));
    }
    return json_;
}


// cJSON *parse_json_fr_file(char* filename) {
//     cJSON *feedJSON=NULL;
//     if (content && *content) {
//         fdebug("%s\n", "do cJSON Parsing" );
//         parseCJsonFromFile(content, &feedJSON);
//         fdebug("cjson printing: %s\n", cJSON_Print(feedJSON));
//     }
//     return feedJSON;
// }

/* Read a file, parse, render back, etc. */
// int parseCJsonFromFile(char *filename, cJSON **json_ptr)
// {
//     fdy_buff *_buf = fbuf_read_file(filename);
//     parseCJson(_buf->data, json_ptr);
//     fbuf_delete(_buf);
// }


int parseCJson(char *text, cJSON **json_ptr) {
    // cJSON *json_ptr;
    *json_ptr = cJSON_Parse(text);
    if (!(*json_ptr)) {
        flog_err("%s\n", cJSON_GetErrorPtr());
    }

    return 1;
}


/*CSJSON PARSE PARAM*/
int cjson_parse_prm_cb(void* item, char* key, char* val) {
    if (val && *val /*&& !csif_isspace(val)*/) {
        cJSON_AddStringToObject((cJSON*)item, key, val);
    } else {
        cJSON_AddNullToObject((cJSON*)item, key);
    }

    return 1;
}

cJSON *_cjson_parse_param_(char *f_payload) {
    cJSON * item = cJSON_CreateObject();

    if (!item)
        return 0;/* memory fail */

    if (f_payload && *f_payload) {
        item->type = cJSON_Object;
        req_param_separator_(f_payload, item, cjson_parse_prm_cb);
    }
    return item;
}

int req_param_separator_(char* query_str, void* obj, int (*callback)(void* obj, char* key, char* val)) {
    char *r_pos;
    char *l_pos;
    char check_point;

NEW_FIELD:
    r_pos = query_str;
    check_point = AMPERSAND_DELIM;
    if (*query_str) {
        for (; (*query_str && !csif_isspace(query_str) && (AMPERSAND_DELIM != *query_str)); query_str++);
        *query_str ^= check_point = *query_str;// check_point = *query_str; *query_str = 0;

        if (*r_pos) {

            l_pos = r_pos;

            while (*r_pos && (EQ_DELIM != *r_pos))
                r_pos++;

            *r_pos = 0;
            r_pos++;

            if (callback(obj, l_pos, r_pos) == 0)
                return 0; // memory failed

            if (csif_isspace(&check_point)) {
                goto END_FIELD;
            }

            *query_str++ ^= check_point; // *query_str = check_point; query_str++;

            r_pos = query_str; //move the string


            if (*query_str) {
                goto NEW_FIELD;
            }
        }
    }

END_FIELD:
    return 1;
}

/*JSON PARSE PARAM*/
void* cjson_get_value(cJSON *item, const char* key) {
    return cJSON_GetObjectItem(item, key);
}

void* cjson_get_index(cJSON *item, int i) {
    return cJSON_GetArrayItem(item, i);
}

/***Incubating****/
void* create_json_obj(void) {
    return csif_create_cjson_obj();
}

void* create_json_arr(void) {
    return csif_create_cjson_arr();
}

void* csif_create_cjson_obj(void) {

    cJSON *obj = cJSON_CreateObject();
    if (!obj) {
        flog_err("%s\n", "Unable to init at feed_create_cjson_obj");
        return NULL;
    }

    obj->type = cJSON_Object;

    return obj;

}

void* csif_create_cjson_arr(void) {
    cJSON *arr = cJSON_CreateArray();

    if (!arr) {
        flog_err("%s\n", "Unable to init at feed_create_cjson_arr");
        return NULL;
    }
    return arr;
}

/***JSON Common****/
