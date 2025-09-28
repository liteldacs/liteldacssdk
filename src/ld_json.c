//
// Created by jiaxv on 23-10-11.
//

#include "ld_json.h"

#include "ld_log.h"

uint64_t double_u64_converter(double val) {
    //uint64_t converted_value = 0;
    //memcpy(&converted_value, &val, sizeof(uint64_t));
    //return converted_value;
    return (uint64_t) val;
}

double u64_double_converter(uint64_t val) {
    double converted_value = 0.0;
    memcpy(&converted_value, &val, sizeof(double));
    return converted_value;
}

/**
 * Serialized the struct which is described by description
 * @param struct_ptr    Pointer of completed struct
 * @param tmpl_desc     Description
 * @return              Cjson struct of root
 */
cJSON *marshel_json(const void *struct_ptr, json_tmpl_desc_t *tmpl_desc) {
    json_tmpl_t *tmpl;
    uint8_t *ptr = (uint8_t *) struct_ptr;

    cJSON *init_obj = cJSON_CreateObject();
    for (tmpl = tmpl_desc->tmpl;; tmpl++) {
        switch (tmpl->cJSON_type) {
            case cJSON_Number: {
                double n = 0;
                switch (tmpl->size) {
                    case 1:
                        n = *(const uint8_t *) ptr;
                        break;
                    case 2:
                        n = *(const uint16_t *) ptr;
                        break;
                    case 4:
                        n = *(const uint32_t *) ptr;
                        break;
                    case 8:
                        n = *(const uint64_t *) ptr;
                        break;
                    default:
                        //应有错误处理
                        break;
                }

                if (tmpl->desc) {
                    n = u64_double_converter((uint64_t) n);
                }


                cJSON_AddNumberToObject(init_obj, tmpl->key, n);
                break;
            }
            case cJSON_String: {
                buffer_t *buf_p = (buffer_t *) *(uint64_t *) ptr;
                cJSON_AddStringToObject(init_obj, tmpl->key, (char *) buf_p->ptr);
                break;
            }
            case cJSON_Invalid: {
                return init_obj;
            }
            case cJSON_Object: {
                if (tmpl->desc) {
                    cJSON *obj = marshel_json((void *) *(uint64_t *) ptr, (json_tmpl_desc_t *) tmpl->desc);
                    cJSON_AddItemToObject(init_obj, tmpl->key, obj);
                }
                break;
            }
            case cJSON_Raw: {
                cJSON_AddRawToObject(init_obj, tmpl->key, (const char *) *(uint64_t *) ptr);
                break;
            }
            case cJSON_Array: {
                cJSON *arr_node = cJSON_CreateArray();
                cJSON **ele_node = (cJSON **) ptr;
                for (int i = 0; ele_node[i] != NULL; i++) {
                    cJSON_AddItemToArray(arr_node, ele_node[i]);
                }
                cJSON_AddItemToObject(init_obj, tmpl->key, arr_node);
                break;
            }

            default:
                break;
        }
        ptr += tmpl->size;
    }
}

/**
 * Deserialized the struct which is described by description
 * @param root          Root of current json need to be parse.
 * @param struct_ptr    Pointer of the empty struct
 * @param tmpl_desc     Description
 */
void unmarshel_json(const cJSON *root, void *struct_ptr, json_tmpl_desc_t *tmpl_desc) {
    json_tmpl_t *tmpl;

    uint8_t *ptr = struct_ptr;

    for (tmpl = tmpl_desc->tmpl;; tmpl++) {
        cJSON *child = cJSON_GetObjectItem(root, tmpl->key);
        do {
            if (tmpl->key != NULL && child == NULL) break;
            switch (tmpl->cJSON_type) {
                case cJSON_Number: {
                    double d;
                    uint64_t n;
                    d = cJSON_GetNumberValue(child);

                    if (tmpl->desc) {
                        *(double *) ptr = d;
                    } else {
                        n = double_u64_converter(d);
                        switch (tmpl->size) {
                            case 1:
                                *(uint8_t *) ptr = n;
                                break;
                            case 2:
                                *(uint16_t *) ptr = n;
                                break;
                            case 4:
                                *(uint32_t *) ptr = n;
                                break;
                            case 8:
                                *(uint64_t *) ptr = n;
                                break;
                            default:
                                //TODO: 错误处理
                                break;
                        }
                    }

                    break;
                }
                case cJSON_String: {
                    buffer_t *buf_p = (buffer_t *) *(uint64_t *) ptr;
                    if (!buf_p) {
                        buf_p = init_buffer_unptr();
                        memcpy(ptr, &buf_p, sizeof(buffer_t *));
                    }

                    char *str = cJSON_GetStringValue(child);
                    CLONE_TO_CHUNK(*buf_p, (uint8_t *)str, strlen(str))

                    break;
                }
                case cJSON_Object: {
                    if (tmpl->desc) {
                        json_tmpl_desc_t *desc = (json_tmpl_desc_t *) tmpl->desc;

                        /**
                         * Init a certain size of block, to store the child struct.
                         * and make the the root struct store the very first address of this child struct.
                         * Which is amazing, i believe.
                        */
                        void *p = (void *) malloc(desc->size);
                        memcpy(ptr, &p, sizeof(void *));

                        unmarshel_json(child, &p, desc);
                    }
                    break;
                }
                case cJSON_Raw: {
                    char *unf = cJSON_PrintUnformatted(child);
                    memcpy(ptr, &unf, sizeof(void *));
                    break;
                }
                case cJSON_Array: {
                    int array_size = cJSON_GetArraySize(child);

                    cJSON **array_p = (cJSON **) ptr;
                    for (int i = 0; i < array_size; i++) {
                        array_p[i] = cJSON_GetArrayItem(child, i);
                    }
                    break;
                }
                case cJSON_Invalid:
                    return;
            }
        } while (0);
        ptr += tmpl->size;
    }
}

json_tmpl_t *get_desc_by_key(json_tmpl_desc_t *desc, const char *key) {
    json_tmpl_t *tmpls = desc->tmpl;
    while (tmpls->cJSON_type != cJSON_Invalid) {
        if (strcmp(tmpls->key, key) == 0)
            return tmpls;
        tmpls++;
    }
    return NULL;
}

l_err get_json_str(void *ptr, json_tmpl_desc_t *desc, char **j_str) {
    if (!desc) {
        log_warn("Description is NULL");
        return LD_ERR_INTERNAL;
    }
    cJSON *data_json = marshel_json(ptr, desc);
    *j_str = cJSON_PrintUnformatted(data_json);
    cJSON_Delete(data_json);
    return LD_OK;
}

buffer_t * get_json_buffer(int type, cJSON *node) {
    char *out_str = NULL;
    buffer_t *buf = init_buffer_unptr();
    switch (type) {
        case JSON_UNFORMAT:
            out_str = cJSON_PrintUnformatted(node);
            break;
        case JSON_FORMAT:
            out_str = cJSON_Print(node);
            break;
        default:
            return NULL;
    }

    CLONE_TO_CHUNK(*buf, out_str, strlen(out_str));
    cJSON_free(out_str);
    return buf;
}
