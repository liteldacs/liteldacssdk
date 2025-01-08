//
// Created by 邹嘉旭 on 2023/12/8.
//
#include "../global/ldacs_sim.h"
#include  "ld_json.h"
#include "ld_util_def.h"
#include "ld_log.h"


#pragma pack(1)

struct outter_json {
    uint8_t k;
    cJSON *array[5];
};

struct inner_json {
    uint8_t m;
};

#pragma pack()

json_tmpl_t outter_tmpl[] = {
    {cJSON_Number, sizeof(uint8_t), "k", "SNP_State", NULL},
    {cJSON_Array, sizeof(void *), "eles", "ELES", NULL},
    {cJSON_Invalid, 0, NULL, NULL, NULL}
};

json_tmpl_t inner_tmpl[] = {
    {cJSON_Number, sizeof(uint8_t), "m", "SNP_State", NULL},
    {cJSON_Invalid, 0, NULL, NULL, NULL}
};

json_tmpl_desc_t outter_tmpl_desc = {
    .desc = "STATE_JSON_TEMPLATE",
    .tmpl = outter_tmpl,
    .size = sizeof(struct outter_json)
};

json_tmpl_desc_t inner_tmpl_desc = {
    .desc = "STATE_JSON_TEMPLATE",
    .tmpl = inner_tmpl,
    .size = sizeof(struct inner_json)
};

int main() {

    struct inner_json inner1 = {
        .m = 1,
    };

    struct outter_json outter = {
        .k = 2,
    };
    struct outter_json outter2;
    zero(&outter2);


    cJSON *inner1_n = marshel_json(&inner1, &inner_tmpl_desc);
    cJSON *inner2_n = marshel_json(&inner1, &inner_tmpl_desc);

    outter.array[0] = inner1_n;
    outter.array[1] = inner2_n;

    cJSON *a = marshel_json(&outter, &outter_tmpl_desc);

    char *jstr = cJSON_PrintUnformatted(a);
    log_warn("%s", jstr);
    cJSON_Delete(a);



    cJSON *b = cJSON_Parse(jstr);
    unmarshel_json(b, &outter2, &outter_tmpl_desc);
    log_warn("%d", outter2.k);


    cJSON_Delete(b);
    free(jstr);

    exit(0);
}
