#include <stdio.h>
#include <sqlite3.h>

int main() {
    sqlite3 *db;
    char *err_msg = 0;
    int rc;

    // 打开数据库连接
    rc = sqlite3_open("/home/jiaxv/.ldcauc/as_keys_001162345.db", &db);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    // 执行 SELECT * 查询
    const char *sql = "SELECT * FROM as_keystore;";
    sqlite3_stmt *stmt;

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    if (rc == SQLITE_OK) {
        int cols = sqlite3_column_count(stmt);

        // 遍历结果集
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            for (int i = 0; i < cols; i++) {
                printf("%s\t", sqlite3_column_text(stmt, i));
            }
            printf("\n");
        }
    } else {
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
    }

    // 清理资源
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return 0;
}
