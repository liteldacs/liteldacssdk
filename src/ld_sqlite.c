//
// Created by 邹嘉旭 on 2024/6/21.
//
#include "ld_sqlite.h"

static void get_current_time(char *time_str) {
    struct tm local_time;
    struct timeval tv;
    char buf[32] = {0};

    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &local_time);
    buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &local_time)] = '\0';

    sprintf(time_str, "%s.%06ld", buf, tv.tv_usec);
}

static l_err open_database(sqlite_entity_t *obj) {
    if (sqlite3_open(obj->db_path, &obj->db)) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(obj->db));
        return LD_ERR_OPEN_SQL;
    }
    return LD_OK;
}

static void close_database(sqlite_entity_t *obj) {
    if (obj->db)
        sqlite3_close(obj->db);
}

static l_err exec_statement(sqlite_entity_t *obj, const char *statement, int (*callback)(void *, int, char **, char **),
                            void *arg) {
    char *err_msg;
    if (sqlite3_exec(obj->db, statement, callback, arg, &err_msg)) {
        log_error("SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        // sqlite3_close(obj->db); // 关闭数据库
        close_database(obj);
        return LD_ERR_FAIL_EXEC_SQL;
    }
    return LD_OK;
}

// static int find_table_columns_cb(void *data, int argc, char **argv, char **column_names) {
//     table_create_data_t *c_data = data;
//      return 0;
// }
//
static int find_table_cb(void *data, int argc, char **argv, char **column_names) {
    table_create_data_t *c_data = data;
    char *st = NULL;
    char **result;
    int nrow, ncol = 0;
    char *errmsg;
    c_data->is_find = argc;

    if (argc) {
        st = sqlite3_mprintf("PRAGMA table_info(%q)", c_data->table_name);
        sqlite3_get_table(c_data->obj->db, st, &result, &nrow, &ncol, &errmsg);
        c_data->column_num = nrow;


        /* get_table第一行是表头，需要从第二行开始处理 */
        for (int i = 0; i < nrow; i++) {
            /* 如果是主键(自增)，则跳过，复制的时候会自动生成新的键值 */
            if (*result[((i + 1) * ncol) + 5] == '1') continue;

            /* 第二列为列名 */
            char *col_name = result[((i + 1) * ncol) + 1];
            /* +1 for '\0' */
            c_data->columns[i] = calloc(strlen(col_name + 1), sizeof(void *));
            strncpy(c_data->columns[i], col_name, strlen(col_name));
        }

        sqlite3_free_table(result);
        sqlite3_free(st);
    }
    return 0;
}

sqlite_entity_t *init_sqlite_entity(const char *db_path) {
    sqlite_entity_t *obj = malloc(sizeof(sqlite_entity_t));
    obj->db_path = db_path;
    return obj;
}

void free_sqlite_entity(sqlite_entity_t *obj) {
    free(obj);
}

void clear_sqlite_obj(void *struct_ptr, table_tmpl_desc_t *t_desc) {
    uint8_t *ptr = (uint8_t *) struct_ptr;
    for (table_tmpl_t *cur_field = t_desc->fields; cur_field->sql_type != sql_end;
         ptr += cur_field->size, cur_field++) {
        switch (cur_field->sql_type) {
            case sql_text: {
                char *buf_p = (char *) *(uint64_t *) ptr;
                if (buf_p != NULL) {
                    free(buf_p);
                    *(uint64_t *) ptr = 0;
                }
                break;
            }
            default:
                break;
        }
    }
}


l_err create_table(sqlite_entity_t *obj, table_tmpl_desc_t *t_desc) {
    l_err err;
    char *statement = NULL;
    size_t state_size = 0;
    FILE *s_stream = open_memstream(&statement, &state_size);
    table_tmpl_t *cur_field = t_desc->fields;

    if ((err = open_database(obj))) return err;

    char select_pre_st[128] = {0};
    sprintf(select_pre_st, "SELECT name FROM sqlite_master WHERE type='table' AND name='%s';", t_desc->table_name);

    table_create_data_t c_data = {
        .obj = obj,
        .table_name = t_desc->table_name,
        .is_find = FALSE,
        .column_num = 0,
    };
    if ((err = exec_statement(obj, select_pre_st, find_table_cb, &c_data))) {
        return err;
    }

    // 根据描述，组装创建表的sql语句
    fprintf(s_stream, "CREATE TABLE IF NOT EXISTS %s_TEMP (\n", t_desc->table_name);
    while (TRUE) {
        fprintf(s_stream, "    %s %s", cur_field->name, sql_field_typ_str[cur_field->sql_type]);
        if (cur_field->primary) fprintf(s_stream, " %s", "PRIMARY KEY");
        if (cur_field->auto_increment) fprintf(s_stream, " %s", "AUTOINCREMENT");
        if (cur_field->not_null) fprintf(s_stream, " %s", "NOT NULL");

        cur_field++;
        if (cur_field->sql_type != sql_end) {
            fprintf(s_stream, ",\n");
        } else {
            break;
        }
    }
    fprintf(s_stream, ");\n");
    fclose(s_stream);
    log_warn("%s", statement);
    if ((err = exec_statement(obj, statement, NULL, NULL))) {
        free(statement);
        return err;
    }
    free(statement); // 释放内存

    /* 判断库中是否存在同名表，若有则将表中对应字段数据复制进新的'_TEMP'表中 */
    if (c_data.is_find == TRUE) {
        /* 寻找重合的字段 */
        char *valid_names[64] = {0};
        int name_num = 0;
        char *col_st = NULL;
        size_t col_st_size;

        /* 逐一匹配，找出新表和旧表字段名一样的字段 */
        for (int i = 0; i < c_data.column_num; i++) {
            if (c_data.columns[i] == NULL) continue;
            for (cur_field = t_desc->fields; cur_field->sql_type != sql_end; cur_field++) {
                if (strcmp(c_data.columns[i], cur_field->name) == 0) {
                    *(valid_names + name_num) = c_data.columns[i];
                    name_num++;
                }
            }
        }

        FILE *columns_strm = open_memstream(&col_st, &col_st_size);
        for (int n = 0; n < name_num; n++) {
            fprintf(columns_strm, "%s", valid_names[n]);
            if (n != name_num - 1) fprintf(columns_strm, ",");
        }
        fclose(columns_strm);
        table_columns_free(&c_data);

        char ins_st[128] = {0};
        sprintf(ins_st, "INSERT INTO %s_TEMP(%s) SELECT %s FROM %s;", t_desc->table_name, col_st, col_st,
                t_desc->table_name);
        if (exec_statement(obj, ins_st, NULL, NULL)) {
            return LD_ERR_FAIL_EXEC_SQL;
        }
        free(col_st);

        /* 删除旧表 */
        char drop_st[128] = {0};
        sprintf(drop_st, "DROP TABLE %s;", t_desc->table_name);
        if (exec_statement(obj, drop_st, NULL, NULL)) {
            return LD_ERR_FAIL_EXEC_SQL;
        }
    }

    /* ‘_TEMP’表作为新表，进行改名 */
    char alt_st[128] = {0};
    sprintf(alt_st, "ALTER TABLE '%s_TEMP' RENAME TO '%s';",
            t_desc->table_name, t_desc->table_name);

    if ((err = exec_statement(obj, alt_st, NULL, NULL))) {
        return err;
    }

    close_database(obj);

    return LD_OK;
}

l_err insert_value(sqlite_entity_t *obj, table_tmpl_desc_t *t_desc, void *struct_ptr) {
    char *statement = NULL;
    size_t state_size = 0;
    l_err err = LD_OK;

    table_tmpl_t *cur_field = t_desc->fields;
    FILE *s_stream = open_memstream(&statement, &state_size);
    uint8_t *ptr = (uint8_t *) struct_ptr;

    fprintf(s_stream, "INSERT INTO %s (", t_desc->table_name);
    while (TRUE) {
        //如果是自增，则跳过，由数据库做自增
        if (cur_field->auto_increment == TRUE) {
            cur_field++;
            continue;
        }
        fprintf(s_stream, "%s", cur_field->name);
        cur_field++;
        if (cur_field->sql_type != sql_end) {
            fprintf(s_stream, ", ");
        } else {
            fprintf(s_stream, ") VALUES (");
            break;
        }
    }

    for (cur_field = t_desc->fields;;) {
        //如果是自增，则跳过，由数据库做自增
        if (cur_field->auto_increment == TRUE) {
            ptr += cur_field->size;
            cur_field++;
            continue;
        }
        switch (cur_field->sql_type) {
            case sql_integer: {
                switch (cur_field->size) {
                    case 1:
                        fprintf(s_stream, "%hhu", *(const uint8_t *) ptr);
                        break;
                    case 2:
                        fprintf(s_stream, "%hu", *(const uint16_t *) ptr);
                        break;
                    case 4:
                        fprintf(s_stream, "%u", *(const uint32_t *) ptr);
                        break;
                    case 8:
                        fprintf(s_stream, "%lu", *(const uint64_t *) ptr);
                        break;
                    default:
                        //应有错误处理
                        break;
                }
                break;
            }
            case sql_real: {
                fprintf(s_stream, "%f", *(const double *) ptr);
                break;
            }
            case sql_text: {
                char *buf_p = (char *) *(uint64_t *) ptr;
                if (strcmp(cur_field->name, "CreatedAt") == 0 || strcmp(cur_field->name, "UpdatedAt") == 0) {
                    char time_str[32] = {0};
                    get_current_time(time_str);

                    fprintf(s_stream, "'%s'", time_str);
                    break;
                } else {
                    if (buf_p == NULL) {
                        fprintf(s_stream, "NULL");
                    } else {
                        fprintf(s_stream, "'%s'", buf_p);
                    }
                    break;
                }
            }
            case sql_null: {
            }
            case sql_blob: {
            }
            case sql_end:
            default:
                break;
        }

        ptr += cur_field->size;
        cur_field++;
        if (cur_field->sql_type != sql_end && cur_field->auto_increment != TRUE) {
            fprintf(s_stream, ", ");
        } else {
            fprintf(s_stream, ");");
            break;
        }
    }

    fclose(s_stream);
    log_error("%s", statement);
    // 执行创建表的SQL语句
    if ((err = open_database(obj))) {
        free(statement);
        return err;
    }

    if ((err = exec_statement(obj, statement, NULL, NULL))) {
        return err;
    }

    sqlite3_close(obj->db); // 关闭数据库
    free(statement); // 释放内存

    return err;
}


l_err update_value(sqlite_entity_t *obj, table_tmpl_desc_t *t_desc, char **key, size_t key_sz, void *value,
                   char **wkey, size_t wkey_sz, void *wvalue, sql_comp *comps, sql_logic *logics) {
    char *statement = NULL;
    size_t state_size = 0;
    l_err err = LD_OK;

    table_tmpl_t *cur_field;
    FILE *s_stream = open_memstream(&statement, &state_size);
    uint8_t *ptr = (uint8_t *) value;

    fprintf(s_stream, "UPDATE %s SET ", t_desc->table_name);
    int i = 0;
    for (cur_field = t_desc->fields; cur_field->sql_type != sql_end && i < key_sz;
         ptr += cur_field->size, cur_field++) {
        /* 按desc对待修改列名进行匹配， 确定修改方式 */
        if (strcmp(cur_field->name, key[i]) == 0) {
            fprintf(s_stream, "%s = ", key[i]);

            switch (cur_field->sql_type) {
                case sql_integer: {
                    switch (cur_field->size) {
                        case 1:
                            fprintf(s_stream, "%hhu", *(const uint8_t *) ptr);
                            break;
                        case 2:
                            fprintf(s_stream, "%hu", *(const uint16_t *) ptr);
                            break;
                        case 4:
                            fprintf(s_stream, "%u", *(const uint32_t *) ptr);
                            break;
                        case 8:
                            fprintf(s_stream, "%lu", *(const uint64_t *) ptr);
                            break;
                        default:
                            //应有错误处理
                            break;
                    }
                    break;
                }
                case sql_real: {
                    fprintf(s_stream, "%f", *(const double *) ptr);
                    break;
                }
                case sql_text: {
                    char *buf_p = (char *) *(uint64_t *) ptr;
                    if (strcmp(cur_field->name, "UpdatedAt") == 0) {
                        break;
                    } else {
                        if (buf_p == NULL) {
                            fprintf(s_stream, "NULL");
                        } else {
                            fprintf(s_stream, "'%s'", buf_p);
                        }
                        break;
                    }
                }
                case sql_null: {
                }
                case sql_blob: {
                }
                case sql_end:
                default:
                    break;
            }
            /* 无需区分" "以及 ", "因为最后一个永远是UpdateAt */
            fprintf(s_stream, ", ");
            i++;
        }
    }

    char time_str[32] = {0};
    get_current_time(time_str);
    fprintf(s_stream, "UpdatedAt = '%s' ", time_str);

    if (wkey == NULL) goto execute;
    fprintf(s_stream, "WHERE ");

    ptr = (uint8_t *) wvalue;
    for (cur_field = t_desc->fields, i = 0; cur_field->sql_type != sql_end && i < wkey_sz;
         ptr += cur_field->size, cur_field++) {
        if (strcmp(cur_field->name, wkey[i]) == 0) {
            fprintf(s_stream, "%s ", wkey[i]);
            if (comps == NULL) {
                fprintf(s_stream, "= ");
            } else {
                fprintf(s_stream, "%s ", sql_comp_str[comps[i]]);
            }

            switch (cur_field->sql_type) {
                case sql_integer: {
                    switch (cur_field->size) {
                        case 1:
                            fprintf(s_stream, "%hhu", *(const uint8_t *) ptr);
                            break;
                        case 2:
                            fprintf(s_stream, "%hu", *(const uint16_t *) ptr);
                            break;
                        case 4:
                            fprintf(s_stream, "%u", *(const uint32_t *) ptr);
                            break;
                        case 8:
                            fprintf(s_stream, "%lu", *(const uint64_t *) ptr);
                            break;
                        default:
                            //应有错误处理
                            break;
                    }
                    break;
                }
                case sql_real: {
                    fprintf(s_stream, "%f", *(const double *) ptr);
                    break;
                }
                case sql_text: {
                    char *buf_p = (char *) *(uint64_t *) ptr;
                    if (buf_p != NULL) {
                        fprintf(s_stream, "'%s'", buf_p);
                    }
                    break;
                }
                case sql_null: {
                }
                case sql_blob: {
                }
                case sql_end:
                default:
                    break;
            }
            i++;
            /* 默认为AND条件 */
            if (i < wkey_sz) {
                if (logics == NULL) {
                    fprintf(s_stream, " AND ");
                } else {
                    fprintf(s_stream, " %s ", sql_logics_str[logics[i - 1]]);
                }
            }
        }
    }

execute: {
        fclose(s_stream);
        log_error("%s", statement);

        if ((err = open_database(obj))) {
            free(statement);
            return err;
        }

        if ((err = exec_statement(obj, statement, NULL, NULL))) {
            return err;
        }

        sqlite3_close(obj->db);
        free(statement);
    }
    return err;
}

l_err select_value(sqlite_entity_t *obj, table_tmpl_desc_t *t_desc, char **key, size_t key_sz, char **wkey,
                   size_t wkey_sz, void *wvalue, sql_comp *comps, sql_logic *logics, void **res, size_t *res_sz) {
    char *statement = NULL;
    size_t state_size = 0;
    l_err err = LD_OK;

    table_tmpl_t *cur_field;
    FILE *s_stream = open_memstream(&statement, &state_size);
    uint8_t *ptr = (uint8_t *) wvalue;
    int i;

    fprintf(s_stream, "SELECT ");
    if (key == NULL && key_sz == 0) {
        fprintf(s_stream, "* ");
    } else {
        for (cur_field = t_desc->fields, i = 0; cur_field->sql_type != sql_end && i < key_sz;
             ptr += cur_field->size, cur_field++) {
            /* 按desc对待修改列名进行匹配， 确定修改方式 */
            if (strcmp(cur_field->name, key[i]) == 0) {
                fprintf(s_stream, "%s", key[i]);
                i++;

                /* 需区分" "以及 ", "*/
                if (i != key_sz) {
                    fprintf(s_stream, ", ");
                } else {
                    fprintf(s_stream, " ");
                }
            }
        }
    }

    fprintf(s_stream, "FROM %s ", t_desc->table_name);

    if (wkey == NULL) goto execute;
    fprintf(s_stream, "WHERE ");

    ptr = (uint8_t *) wvalue;
    for (cur_field = t_desc->fields, i = 0; cur_field->sql_type != sql_end && i < wkey_sz;
         ptr += cur_field->size, cur_field++) {
        if (strcmp(cur_field->name, wkey[i]) == 0) {
            fprintf(s_stream, "%s ", wkey[i]);
            if (comps == NULL) {
                fprintf(s_stream, "= ");
            } else {
                fprintf(s_stream, "%s ", sql_comp_str[comps[i]]);
            }

            switch (cur_field->sql_type) {
                case sql_integer: {
                    switch (cur_field->size) {
                        case 1:
                            fprintf(s_stream, "%hhu", *(const uint8_t *) ptr);
                            break;
                        case 2:
                            fprintf(s_stream, "%hu", *(const uint16_t *) ptr);
                            break;
                        case 4:
                            fprintf(s_stream, "%u", *(const uint32_t *) ptr);
                            break;
                        case 8:
                            fprintf(s_stream, "%lu", *(const uint64_t *) ptr);
                            break;
                        default:
                            //应有错误处理
                            break;
                    }
                    break;
                }
                case sql_real: {
                    fprintf(s_stream, "%f", *(const double *) ptr);
                    break;
                }
                case sql_text: {
                    char *buf_p = (char *) *(uint64_t *) ptr;
                    if (buf_p != NULL) {
                        fprintf(s_stream, "'%s'", buf_p);
                    }
                    break;
                }
                case sql_null: {
                }
                case sql_blob: {
                }
                case sql_end:
                default:
                    break;
            }
            i++;
            if (i < wkey_sz) {
                if (logics == NULL) {
                    fprintf(s_stream, " AND ");
                } else {
                    fprintf(s_stream, " %s ", sql_logics_str[logics[i - 1]]);
                }
            }
        }
    }

execute: {
        sqlite3_stmt *stmt;
        int step_ret;

        fclose(s_stream);
        log_error("%s", statement);

        if ((err = open_database(obj))) {
            free(statement);
            return err;
        }
        if (sqlite3_prepare_v2(obj->db, statement, -1, &stmt, NULL) != SQLITE_OK) {
            sqlite3_close(obj->db);
            return 1;
        }


        // 执行查询并处理结果
        while ((step_ret = sqlite3_step(stmt)) == SQLITE_ROW) {
            void *out_struct = calloc(1, t_desc->size);
            uint8_t *out_ptr = out_struct;

            int j = 0;
            for (cur_field = t_desc->fields, i = 0; cur_field->sql_type != sql_end;
                 out_ptr += cur_field->size, cur_field++) {
                /* 当为全部查询 (SELECT *)时，无需筛选字段，否则需要通过匹配name对字段进行筛选 */
                if (key != NULL && key_sz != 0) {
                    if (strcmp(cur_field->name, key[i]) != 0) continue;
                }

                switch (cur_field->sql_type) {
                    case sql_integer: {
                        int64_t n = sqlite3_column_int64(stmt, j);
                        switch (cur_field->size) {
                            case 1:
                                *(int8_t *) out_ptr = n;
                                break;
                            case 2:
                                *(int16_t *) out_ptr = n;
                                break;
                            case 4:
                                *(int32_t *) out_ptr = n;
                                break;
                            case 8:
                                *(int64_t *) out_ptr = n;
                                break;
                            default:
                                //应有错误处理
                                break;
                        }
                        break;
                    }
                    case sql_real: {
                        *(double *) out_ptr = sqlite3_column_double(stmt, j);
                        break;
                    }
                    case sql_text: {
                        const uint8_t *text = sqlite3_column_text(stmt, j);
                        if (text == NULL) {
                            *(uint64_t *) out_ptr = 0;
                        } else {
                            size_t text_len = strlen((char *) text);
                            uint8_t *ntext = calloc(text_len, sizeof(uint8_t));
                            memcpy(ntext, text, text_len);
                            memcpy(out_ptr, &ntext, sizeof(void *));
                        }
                        break;
                    }
                    case sql_null: {
                    }
                    case sql_blob: {
                    }
                    case sql_end:
                    default:
                        break;
                }
                j++;
                if (key != NULL && key_sz != 0) {
                    i++;
                    if (i == key_sz) break;
                }
            }
            res[*res_sz] = out_struct;
            (*res_sz)++;
        }

        sqlite3_finalize(stmt);
    }
    sqlite3_close(obj->db);
    free(statement);
    return LD_OK;
}


l_err delete_value(sqlite_entity_t *obj, table_tmpl_desc_t *t_desc, size_t wkey_sz, void *wvalue) {
    return LD_OK;
}
