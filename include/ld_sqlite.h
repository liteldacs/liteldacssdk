//
// Created by 邹嘉旭 on 2024/6/21.
//

#ifndef LD_SQLITE_H
#define LD_SQLITE_H

#include "../global/ldacs_sim.h"
#include "ld_log.h"

typedef struct table_tmpl_s table_tmpl_t;
typedef struct table_tmpl_desc_s table_tmpl_desc_t;
typedef struct table_foreign_key_s table_foreign_key_t;

typedef struct sqlite_obj_s {
    const char *db_path;
    sqlite3 *db;
} sqlite_entity_t;

typedef enum sql_field_type_e {
    sql_integer = 0,
    sql_real,
    sql_text,
    sql_blob,
    sql_null,
    sql_end,
} sql_field_type;

typedef enum sql_logics_e {
    SQL_AND = 0,
    SQL_OR,
    SQL_NOT
} sql_logic;

const static char *sql_logics_str[] = {
    "AND",
    "OR",
    "NOT"
};

typedef enum sql_comp_e {
    SQL_EQ = 0,
    SQL_NEQ,
    SQL_LT,
    SQL_LE,
    SQL_MT,
    SQL_ME,
} sql_comp;

const static char *sql_comp_str[] = {
    "=",
    "<>",
    "<",
    "<=",
    ">",
    ">=",
};

static const char *sql_field_typ_str[] = {
    "INTEGER",
    "REAL",
    "TEXT",
    "BLOB",
    "NULL",
    "",
};

struct table_foreign_key_s {
    struct table_tmpl_desc_s *foreign_desc;
    const char *ref;
};

typedef struct table_create_data_s {
    struct sqlite_obj_s *obj;
    const char *table_name;
    bool is_find;
    char *columns[64];
    int column_num;
} table_create_data_t;

static void table_columns_free(const table_create_data_t *data) {
    for (int i = 0; i < data->column_num; i++) {
        if (data->columns[i])
            free(data->columns[i]);
    }
}

struct table_tmpl_s {
    sql_field_type sql_type;
    size_t size;
    const char *name;
    const char *intro;
    bool primary;
    bool auto_increment;
    bool not_null;
    // table_foreign_key_t foreign_key;
};

struct table_tmpl_desc_s {
    const char *table_name;
    const char *desc;
    struct table_tmpl_s *fields;
    size_t size;
};


#define PREFIX_MODEL_DESC \
    {sql_integer, sizeof(uint64_t), "ID", "", TRUE, TRUE},\
    {sql_text, sizeof(void *), "CreatedAt", ""},\
    {sql_text, sizeof(void *), "UpdatedAt", ""},\
    {sql_text, sizeof(void *), "DeletedAt", ""},

#define PREFIX_MODEL \
    uint64_t id;\
char *create_time;\
char *update_time;\
char *delete_time;


sqlite_entity_t *init_sqlite_entity(const char *db_path);

void free_sqlite_entity(sqlite_entity_t *obj);

void clear_sqlite_obj(void *struct_ptr, table_tmpl_desc_t *t_desc);

l_err create_table(sqlite_entity_t *obj, table_tmpl_desc_t *t_desc);

l_err insert_value(sqlite_entity_t *obj, table_tmpl_desc_t *t_desc, void *struct_ptr);

/**
 * 更新表
 * @param obj
 * @param t_desc
 * @param key 更改列名（按desc顺序组织数组）
 * @param key_sz 需更改列的数量
 * @param value 更改的目的值
 * @param wkey 条件列名 （按desc顺序组织数组）
 * @param wkey_sz 条件数量
 * @param wvalue 条件值
 * @param logics 各条件逻辑关系 （size = wkey_sz - 1）,若为空默认为AND
 * @return
 */
l_err update_value(sqlite_entity_t *obj, table_tmpl_desc_t *t_desc, char **key, size_t key_sz, void *value,
                   char **wkey, size_t wkey_sz, void *wvalue, sql_comp *comps, sql_logic *logics);

l_err select_value(sqlite_entity_t *obj, table_tmpl_desc_t *t_desc, char **key, size_t key_sz, char **wkey,
                   size_t wkey_sz, void *wvalue, sql_comp *comps, sql_logic *logics, void **res, size_t *res_sz);


#endif //LD_SQLITE_H
