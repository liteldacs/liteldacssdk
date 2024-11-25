//
// Created by root on 10/23/24.
//
/**
 * @author wencheng
 * @date 2024/07/01
 * @brief rkey gen and export to file
*/
#include <stdio.h>
#include <string.h>
#include "km/key_manage.h"
#include "km/kmdb.h"
#include "ld_log.h"


int main() {
    // 网关端根密钥生成
    const char *dbname = "keystore.db";
    const char *sgw_tablename = "sgw_keystore";
    const char *sgw_name = "SGW";
    const char *as_name = "Berry";
    uint32_t rootkey_len = 16;
//    const char *export_dir = "/home/wencheng/crypto/key_management/keystore/rootkey.bin"; // 导出根密钥到文件
    const char *export_dir = "/root/ldacs/stack_new/ldacs_stack/resources/keystore/rootkey.bin"; // 导出根密钥到文件"; // 导出根密钥到文件
    uint32_t validity_period = 365;                                          // 更新周期365天

    if (km_rkey_gen_export(as_name, sgw_name, rootkey_len, validity_period, dbname, sgw_tablename, export_dir)) {
        log_warn("根密钥生成、保存和导出失败。\n");
    }

    log_warn("OK\n");

    return 0;
}
