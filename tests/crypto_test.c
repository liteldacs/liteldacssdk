//
// Created by root on 5/4/24.
//
#include <stdio.h>
#include <string.h>
#include <km/key_manage.h>


int main() {
#ifdef USE_CRYCARD
    void *DeviceHandle, *pSessionHandle, *phKeyHandle;
    SDF_OpenDevice(&DeviceHandle);                  // 打开设备
    SDF_OpenSession(DeviceHandle, &pSessionHandle); // 打开会话句柄
    unsigned iv_len = 16;
    uint8_t iv_mac[16] = {0x02, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32,
                          0x20};
    uint8_t iv_enc[16] = {0x02, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32,
                          0x20};
    uint8_t iv_dec[16] = {0x02, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32,
                          0x20};
    uint8_t data[32] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
                        0x29, 0xbe, 0xe1, 0xd6, 0x52, 0x49, 0xf1, 0xe9, 0xb3, 0xdb, 0x87, 0x3e, 0x24, 0x0d, 0x06, 0x47};
    unsigned int data_len = 32;
    uint8_t cipher_data[32];
    unsigned int cipher_data_len;
    uint8_t plain_data[32];
    unsigned int plain_data_len;
    int ret;

// 随机数生成
    uint8_t rand[16];
    ret = SDF_GenerateRandom(pSessionHandle, 16, rand);
    if (ret != LD_KM_OK) {
        printf("Error in SDF_GenerateRandom,return : %d\n", ret);
    } else {

        // printbuff("generate random OK", rand, 16);
    }

// kdk生成和导入
    uint32_t kdk_len = 16;
    uint8_t kdk[16];
    ret = SDF_GenerateRandom(pSessionHandle, kdk_len, kdk);
    if (ret != 0) {
        printf("Error in SDF_GenerateRandom for kdk,return : %d\n", ret);
    } else {
        // printbuff("generate kdk OK", kdk, 16);
    }

    void *kdkhandle;
    ret = SDF_ImportKey(pSessionHandle, kdk, kdk_len, &kdkhandle);
    if (ret != LD_KM_OK) {
        printf("Error in SDF_ImportKey,return : %d\n", ret);
    } else {
        // printf("import kdk OK\n");
    }

    // 派生主密钥
    void *master_keyhandle;
    struct KeyPkg m_pkg;
    ret = derive_key(kdkhandle, MASTER_KEY_AS_GS, 32, "AS1", "SGW1", rand, 16, &master_keyhandle, &m_pkg);
    if (ret == LD_KM_OK) {
        printf("derive masterkey OK\n");
        print_key_pkg(&m_pkg);
    } else {
        printf("derive Failed\n");
        return -1;
    }

    // 获取主密钥句柄
    void *m_keyhanle;
    ret = get_keyhandle(&m_pkg, &m_keyhanle);
    if (ret != LD_KM_OK) {
        printf("get_keyhandle Failed\n");
        return -1;
    } else {
        printf("get_keyhandle OK\n");
    }

    // 派生会话密钥
    void *session_keyhandle;
    struct KeyPkg s_pkg;
    ret = derive_key(master_keyhandle, SESSION_KEY_USR_ENC, 16, "AS1", "SGW1", rand, 16, &session_keyhandle, &s_pkg);
    if (ret == LD_KM_OK) {
        printf("derive sessionkey OK\n");
        print_key_pkg(&s_pkg);
    } else {
        printf("derive Failed\n");
        return -1;
    }

    // 会话密钥加解密测试
    unsigned int alg_id = SGD_SM4_ECB;
    int encrypt_result = SDF_Encrypt(pSessionHandle, session_keyhandle, alg_id, iv_enc, data, data_len, cipher_data,
                                     &cipher_data_len);
    if (encrypt_result == 0) {
        // 加密成功，cipher_data 中存放着加密后的数据
        // printbuff("raw data", data, data_len);
        // printbuff("encrypted data", cipher_data, cipher_data_len);
        // printf("Encryption successful!\n");
    } else {
        // 加密失败，根据错误代码进行处理
        printf("Encryption failed with error code: %d\n", encrypt_result);
        return -1;
    }
    int decrypt_result = SDF_Decrypt(pSessionHandle, session_keyhandle, alg_id, iv_dec, cipher_data, cipher_data_len,
                                     plain_data, &plain_data_len);
    if (decrypt_result == 0) {
        //  printbuff("decrypted data", plain_data, plain_data_len);
        //  printf("Decryption successful!\n");
    } else {
        printf("Decryption failed with error code: %d\n", decrypt_result);
        return -1;
    }
    // 校验加解密是否成功
    if (memcmp(plain_data, data, data_len) != 0) {
        printf("en-decrypt data error\n");
        return -1;
    }

    // 销毁内存中的密钥 关闭会话
    SDF_CloseSession(pSessionHandle);
    SDF_CloseDevice(DeviceHandle);

#endif

    return 0;
}
