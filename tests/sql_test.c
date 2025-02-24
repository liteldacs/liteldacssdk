//
// Created by 邹嘉旭 on 2024/6/21.
//
#include "ld_sqlite.h"

#pragma  pack(1)
struct test_sql {
    PREFIX_MODEL
    double d;
    double d2;
    int32_t i;
    char *name;
    char *name2;
};
#pragma  pack()

static table_tmpl_t test_sql_tmpl[] = {
    PREFIX_MODEL_DESC
    {sql_real, sizeof(double), "D", ""},
    {sql_real, sizeof(double), "D2", ""},
    {sql_integer, sizeof(int32_t), "I", ""},
    {sql_text, sizeof(void *), "NAME", ""},
    {sql_text, sizeof(void *), "NAME2", ""},
    {sql_end},
};

static table_tmpl_desc_t test_sql_tmpl_desc = {
    .table_name = "TEST",
    .desc = "TEST TMPL",
    .fields = test_sql_tmpl,
    .size = sizeof(struct test_sql),
};


int main() {
    const char *db_path = "ld_sql.db";
    sqlite_entity_t *sql_obj = init_sqlite_entity(db_path);
    struct test_sql t_sql = {
        .id = 3,
        .d = 35.14,
        .d2 = 1515.81,
        .i = 5810,
        .name = "YiQin",
        .name2 = "YiQin2",
    };

    create_table(sql_obj, &test_sql_tmpl_desc);
    insert_value(sql_obj, &test_sql_tmpl_desc, &t_sql);

    update_value(sql_obj, &test_sql_tmpl_desc, (char *[]){"I", "NAME"}, 2,
                 &(struct test_sql){.i = 65536, .name = "ABCD"}, (char *[]){"ID", "NAME"}, 2,
                 &(struct test_sql){.id = 1, .name = "YiQin"},
                 (sql_comp []){SQL_EQ, SQL_NEQ},
                 (sql_logic []){SQL_OR});

    struct test_sql *sqls[256];
    size_t sz = 0;
    select_value(sql_obj, &test_sql_tmpl_desc, (char *[]){"ID", "CreatedAt", "D2", "I", "NAME", "NAME2"}, 6,
                 (char *[]){"NAME", "NAME2"}, 2,
                 &(struct test_sql){.name = "YiQin", .name2 = "YiQin2"},
                 (sql_comp []){SQL_EQ, SQL_EQ},
                 (sql_logic []){SQL_AND},
                 (void *) sqls, &sz);

    log_warn("%d", sz);
    for (int u = 0; u < sz; u++) {
        struct test_sql *t = sqls[u];

        clear_sqlite_obj(t, &test_sql_tmpl_desc);
        free(t);
    }

    free_sqlite_entity(sql_obj);

    return 0;
}
