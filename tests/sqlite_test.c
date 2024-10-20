//
// Created by 邹嘉旭 on 2024/5/8.
//


#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

// 数据库文件名
const char *DB_FILE = "example.db";

// 回调函数，用于执行查询sql语句后的结果处理
int selectCallback(void *data, int argc, char **argv, char **azColName) {
    int i;
    for (i = 0; i < argc; i++) {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");

    return 0;
}

// 初始化数据库连接
sqlite3 *initDatabase() {
    sqlite3 *db;

    int rc = sqlite3_open(DB_FILE, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "无法打开数据库: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return NULL;
    }

    return db;
}

// 关闭数据库连接
void closeDatabase(sqlite3 *db) {
    if (db) {
        sqlite3_close(db);
    }
}

// 创建表
void createTable(sqlite3 *db) {
    char *errMsg;
    const char *createSql = "CREATE TABLE IF NOT EXISTS students (id INT PRIMARY KEY, name TEXT, age INT);";

    int rc = sqlite3_exec(db, createSql, NULL, 0, &errMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "无法创建表: %s\n", errMsg);
        sqlite3_free(errMsg);
    } else {
        printf("表创建成功\n");
    }
}


// 插入数据
void insertData(sqlite3 *db, int id, const char *name, int age) {
    char insertSql[100];
    snprintf(insertSql, sizeof(insertSql), "INSERT INTO students (id, name, age) VALUES (%d, '%s', %d);", id, name,
             age);

    char *errMsg;
    int rc = sqlite3_exec(db, insertSql, NULL, 0, &errMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "无法插入数据: %s\n", errMsg);
        sqlite3_free(errMsg);
    } else {
        printf("数据插入成功\n");
    }
}

// 更新数据
void updateData(sqlite3 *db, int id, const char *name, int age) {
    char updateSql[100];
    snprintf(updateSql, sizeof(updateSql), "UPDATE students SET name = '%s', age = %d WHERE id = %d;", name, age, id);

    char *errMsg;
    int rc = sqlite3_exec(db, updateSql, NULL, 0, &errMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "无法更新数据: %s\n", errMsg);
        sqlite3_free(errMsg);
    } else {
        printf("数据更新成功\n");
    }
}

// 删除数据
void deleteData(sqlite3 *db, int id) {
    char deleteSql[100];
    snprintf(deleteSql, sizeof(deleteSql), "DELETE FROM students WHERE id = %d;", id);

    char *errMsg;
    int rc = sqlite3_exec(db, deleteSql, NULL, 0, &errMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "无法删除数据: %s\n", errMsg);
        sqlite3_free(errMsg);
    } else {
        printf("数据删除成功\n");
    }
}

// 查询数据
void selectData(sqlite3 *db) {
    char *errMsg;
    const char *selectSql = "SELECT * FROM students;";

    int rc = sqlite3_exec(db, selectSql, selectCallback, 0, &errMsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "无法查询数据: %s\n", errMsg);
        sqlite3_free(errMsg);
    }
}

int main() {
    sqlite3 *db = initDatabase();

    if (db) {
        createTable(db);
        insertData(db, 1, "张三", 20);
        insertData(db, 2, "李四", 22);
        insertData(db, 3, "王五", 25);
        selectData(db);
        updateData(db, 1, "赵六", 23);
        selectData(db);
        deleteData(db, 3);
        selectData(db);
        closeDatabase(db);
    }

    return 0;
}
