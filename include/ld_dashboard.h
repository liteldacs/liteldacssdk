//
// Created by jiaxv on 2025/10/5.
//

#ifndef LITELDACS_SDK_LD_DASHBOARD_H
#define LITELDACS_SDK_LD_DASHBOARD_H

#include "ld_json.h"
#include "ld_net.h"

typedef struct dashboard_obj_s {
    pthread_t conn_th;
    net_ctx_t net_ctx;
    basic_conn_t *conn;
}dashboard_obj_t;

#define BACKEND_IP "127.0.0.1"
#define BACKEND_PORT 9876

typedef enum {
    AS_REGISTER,
    AS_UPDATE_COORDINATE,
    DASHBOARD_START_STOP_AS,
    GS_REGISTER,
    DASHBOARD_SWITCH_AS, // dashboard指导GS切换AS
    AS_GS_RECEIVED_MSG,
    GS_ACCESS_AS,
    GS_AS_EXITED,
    DASHBOARD_SEND_SINGLE_DATA,
    DASHBOARD_SEND_MULTI_DATA,
    CN_REGISTER,
    CN_SIGNALLING,
    DASHBOARD_GET_CN_DATA,


    UNREGISTER_AS = 0xFE,
    UNREGISTER_GS = 0xFF,
}DASHBOARD_FUNCTION;

typedef struct dashboard_func_define_s{
    DASHBOARD_FUNCTION func_e;
    json_tmpl_desc_t *tmpl;
}dashboard_func_define_t;

#pragma pack(1)
typedef struct dashboard_data_s {
    uint8_t type;
    char *data;
} dashboard_data_t;

typedef struct dashboard_update_coordinate_s {
    uint32_t UA;
    double longitude;
    double latitude;
    bool is_direct;
}dashboard_update_coordinate_t;

typedef struct dashboard_register_gs_s {
    uint16_t TAG;
    double longitude;
    double latitude;
}dashboard_register_gs_t;

typedef struct dashboard_register_cn_s {
    uint8_t type;
    uint16_t element;
}dashboard_register_cn_t;

typedef struct dashboard_switch_as_s {
    uint32_t UA;
    uint16_t GST_SAC;
}dashboard_switch_as_t;

typedef struct dashboard_received_msg_s {
    uint8_t orient;
    uint8_t type;
    uint32_t sender;
    uint32_t receiver;
    buffer_t *data;
}dashboard_received_msg_t;

typedef struct dashboard_exit_as_s {
    uint32_t UA;
}dashboard_as_exit_t;

typedef struct dashboard_as_info_upd_s {
    uint32_t UA;
    uint16_t AS_SAC;
    uint16_t GS_SAC;
}dashboard_as_info_upd_t;

typedef struct dashboard_upload_cn_signalling_s {
    uint16_t sender;
    uint16_t receiver;
    uint8_t interface;
    uint8_t signalling;
}dashboard_upload_cn_signalling_t;
#pragma pack()

extern json_tmpl_desc_t dashboard_data_tmpl_desc;

extern const dashboard_func_define_t dashboard_func_defines[];

void *dashboard_conn_connect(net_ctx_t *ctx, char *remote_addr, int remote_port, int local_port);

l_err dashboard_data_send(dashboard_obj_t *dashboad_obj, DASHBOARD_FUNCTION func_e, void *data);

void dashboard_conn_close(basic_conn_t *bc);

l_err init_dashboard(dashboard_obj_t *dashboad_obj, recv_handler recv_handler);


#endif //LITELDACS_SDK_LD_DASHBOARD_H