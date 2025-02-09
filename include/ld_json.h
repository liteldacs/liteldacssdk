//
// Created by jiaxv on 23-10-11.
//

#ifndef LDACS_SIM_UTIL_JSON_H
#define LDACS_SIM_UTIL_JSON_H
#include "../global/ldacs_sim.h"
#include "ld_buffer.h"

#define JSON_UNFORMAT 0
#define JSON_FORMAT 1


typedef struct json_tmpl_s {
    const uint8_t cJSON_type;
    size_t size;
    const char *key;
    const char *intro;
    void *desc;
} json_tmpl_t;

typedef struct json_tmpl_desc_s {
    const char *desc;
    json_tmpl_t *tmpl;
    size_t size;
} json_tmpl_desc_t;

static bool isfloat = TRUE;

json_tmpl_t *get_desc_by_key(json_tmpl_desc_t *desc, const char *key);

cJSON *marshel_json(const void *struct_ptr, json_tmpl_desc_t *tmpl_desc);

void unmarshel_json(const cJSON *root, void *struct_ptr, json_tmpl_desc_t *tmpl_desc);

buffer_t * get_json_buffer(int type, cJSON *node);

void get_json_str(void *ptr, json_tmpl_desc_t *desc, char **j_str);

#endif //LDACS_SIM_UTIL_JSON_H
