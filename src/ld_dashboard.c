//
// Created by jiaxv on 2025/10/5.
//

#include "ld_dashboard.h"


static json_tmpl_t dashboard_data_tmpl[] = {
    {cJSON_Number, sizeof(uint8_t), "type", "function", NULL},
    {cJSON_Raw, sizeof(char *), "data", "data", NULL},
    {cJSON_Invalid, 0, NULL, NULL, NULL}
};

static json_tmpl_t dashboard_update_coordinate_tmpl[] = {
    {cJSON_Number, sizeof(uint32_t), "UA", "UA", NULL},
    {cJSON_Number, sizeof(double), "longitude", "longitude", &isfloat},
    {cJSON_Number, sizeof(double), "latitude", "latitude", &isfloat},
    {cJSON_Number, sizeof(uint8_t), "isDirect", "isDirect", NULL},
    {cJSON_Invalid, 0, NULL, NULL, NULL}
};

static json_tmpl_t dashboard_register_gs_tmpl[] = {
    {cJSON_Number, sizeof(uint16_t), "tag", "tag", NULL},
    {cJSON_Number, sizeof(double), "longitude", "longitude", &isfloat},
    {cJSON_Number, sizeof(double), "latitude", "latitude", &isfloat},
    {cJSON_Invalid, 0, NULL, NULL, NULL}
};

static json_tmpl_t dashboard_register_cn_tmpl[] = {
    {cJSON_Number, sizeof(uint8_t), "type", "type", NULL},
    {cJSON_Number, sizeof(uint16_t), "element", "element", NULL},
    {cJSON_Invalid, 0, NULL, NULL, NULL}
};

static json_tmpl_t dashboard_switch_as_tmpl[] = {
    {cJSON_Number, sizeof(uint32_t), "UA", "UA", NULL},
    {cJSON_Number, sizeof(uint16_t), "gst", "gst", NULL},
    {cJSON_Invalid, 0, NULL, NULL, NULL}
};

static json_tmpl_t dashboard_received_msg_tmpl[] = {
    {cJSON_Number, sizeof(uint8_t), "orient", "orient", NULL},
    {cJSON_Number, sizeof(uint8_t), "type", "type", NULL},
    {cJSON_Number, sizeof(uint32_t), "sender", "sender", NULL},
    {cJSON_Number, sizeof(uint32_t), "receiver", "receiver", NULL},
    {cJSON_String, sizeof(buffer_t *), "message", "message", NULL},
    {cJSON_Invalid, 0, NULL, NULL, NULL}
};

static json_tmpl_t dashboard_as_exit_tmpl[] = {
    {cJSON_Number, sizeof(uint32_t), "UA", "UA", NULL},
    {cJSON_Invalid, 0, NULL, NULL, NULL}
};

static json_tmpl_t dashboard_as_info_upd_tmpl[] = {
    {cJSON_Number, sizeof(uint32_t), "UA", "UA", NULL},
    {cJSON_Number, sizeof(uint16_t), "AS_SAC", "AS_SAC", NULL},
    {cJSON_Number, sizeof(uint16_t), "GS_SAC", "GS_SAC", NULL},
    {cJSON_Invalid, 0, NULL, NULL, NULL}
};

static json_tmpl_t dashboard_upload_cn_signalling_tmpl[] = {
    {cJSON_Number, sizeof(uint16_t), "sender", "sender", NULL},
    {cJSON_Number, sizeof(uint16_t), "receiver", "receiver", NULL},
    {cJSON_Number, sizeof(uint8_t), "interface", "interface", NULL},
    {cJSON_Number, sizeof(uint8_t), "signalling", "signalling", NULL},
    {cJSON_Invalid, 0, NULL, NULL, NULL}
};

static json_tmpl_t dashboard_get_cn_data_tmpl[] = {
    {cJSON_Number, sizeof(uint16_t), "SAC", "SAC", NULL},
    {cJSON_Invalid, 0, NULL, NULL, NULL}
};

static json_tmpl_t dashboard_set_cn_data_tmpl[] = {
    {cJSON_Array, sizeof(void *), "infos", "infos", NULL},
    {cJSON_Invalid, 0, NULL, NULL, NULL}
};

static json_tmpl_t dashboard_accelerate_as_tmpl[] = {
    {cJSON_Number, sizeof(uint8_t), "multiplier", "multiplier", NULL},
    {cJSON_Invalid, 0, NULL, NULL, NULL}
};

json_tmpl_desc_t dashboard_data_tmpl_desc = {
    .desc = "DASHBOARD_DATA",
    .tmpl = dashboard_data_tmpl,
    .size = sizeof(dashboard_data_t)
};

json_tmpl_desc_t dashboard_update_coordinate_tmpl_desc = {
    .desc = "DASHBOARD_UPDATE_COORDINATE",
    .tmpl = dashboard_update_coordinate_tmpl,
    .size = sizeof(dashboard_update_coordinate_t)
};

json_tmpl_desc_t dashboard_register_gs_tmpl_desc = {
    .desc = "DASHBOARD_REGISTER_GS",
    .tmpl = dashboard_register_gs_tmpl,
    .size = sizeof(dashboard_register_gs_t)
};

json_tmpl_desc_t dashboard_register_cn_tmpl_desc = {
    .desc = "DASHBOARD_REGISTER_CN",
    .tmpl = dashboard_register_cn_tmpl,
    .size = sizeof(dashboard_register_cn_t)
};

json_tmpl_desc_t dashboard_switch_as_tmpl_desc = {
    .desc = "DASHBOARD_SWITCH_AS",
    .tmpl = dashboard_switch_as_tmpl,
    .size = sizeof(dashboard_switch_as_t)
};

json_tmpl_desc_t dashboard_received_msg_tmpl_desc = {
    .desc = "DASHBOARD_RECEIVED_MSG",
    .tmpl = dashboard_received_msg_tmpl,
    .size = sizeof(dashboard_received_msg_t)
};

json_tmpl_desc_t dashboard_as_exit_tmpl_desc = {
    .desc = "DASHBOARD_EXIT_AS",
    .tmpl = dashboard_as_exit_tmpl,
    .size = sizeof(dashboard_as_exit_t)
};

static json_tmpl_desc_t dashboard_as_info_upd_tmpl_desc = {
    .desc = "DASHBOARD_AS_INFO",
    .tmpl = dashboard_as_info_upd_tmpl,
    .size = sizeof(dashboard_as_info_upd_t)
};

static json_tmpl_desc_t dashboard_upload_cn_signalling_tmpl_desc = {
    .desc = "DASHBOARD_UPLOAD_CN_SIGNALLING",
    .tmpl = dashboard_upload_cn_signalling_tmpl,
    .size = sizeof(dashboard_upload_cn_signalling_t)
};

static json_tmpl_desc_t dashboard_get_cn_data_tmpl_desc = {
    .desc = "DASHBOARD_GET_CN_DATA",
    .tmpl = dashboard_get_cn_data_tmpl,
    .size = sizeof(dashboard_get_cn_data_t)
};

static json_tmpl_desc_t dashboard_set_cn_data_tmpl_desc = {
    .desc = "DASHBOARD_SET_CN_DATA",
    .tmpl = dashboard_set_cn_data_tmpl,
    .size = sizeof(dashboard_set_cn_data_t)
};

static json_tmpl_desc_t dashboard_accelerate_as_tmpl_desc = {
    .desc = "DASHBOARD_ACCELERATE_AS",
    .tmpl = dashboard_accelerate_as_tmpl,
    .size = sizeof(dashboard_accelerate_as_t)
};

const dashboard_func_define_t dashboard_func_defines[] = {
    {AS_REGISTER, &dashboard_update_coordinate_tmpl_desc},
    {AS_UPDATE_COORDINATE, &dashboard_update_coordinate_tmpl_desc},
    {DASHBOARD_START_STOP_AS, NULL},
    {GS_REGISTER, &dashboard_register_gs_tmpl_desc},
    {DASHBOARD_SWITCH_AS, &dashboard_switch_as_tmpl_desc},
    {AS_GS_RECEIVED_MSG, &dashboard_received_msg_tmpl_desc},
    {GS_ACCESS_AS, &dashboard_as_info_upd_tmpl_desc},
    {GS_AS_EXITED, &dashboard_as_exit_tmpl_desc},
    {DASHBOARD_SEND_SINGLE_DATA, NULL},
    {DASHBOARD_SEND_MULTI_DATA, NULL},
    {CN_REGISTER, &dashboard_register_cn_tmpl_desc},
    {CN_SIGNALLING, &dashboard_upload_cn_signalling_tmpl_desc},
    {DASHBOARD_GET_CN_DATA, &dashboard_get_cn_data_tmpl_desc},
    {CN_SET_CN_DATA, &dashboard_set_cn_data_tmpl_desc},
    {DASHBOARD_ACCELRATE_AS, &dashboard_accelerate_as_tmpl_desc}
};

void *dashboard_conn_connect(net_ctx_t *ctx, char *remote_addr, int remote_port, int local_port) {
    basic_conn_t *bc = malloc(sizeof(basic_conn_t));

    bc->remote_addr = strdup(remote_addr);
    bc->remote_port = remote_port;
    bc->local_port = local_port;

    if (init_basic_conn_client(bc, ctx, LD_TCP_CLIENT) == FALSE) {
        return NULL;
    }

    return bc;
}


l_err dashboard_data_send(dashboard_obj_t *dashboad_obj, DASHBOARD_FUNCTION func_e, void *data) {
    char *data_str = NULL;
    if (get_json_str(data, dashboard_func_defines[func_e].tmpl, &data_str) != LD_OK) {
        log_warn("Cannot generate JSON string");
        return LD_ERR_INTERNAL;
    }

    dashboard_data_t to_resp = {
        .type = (uint8_t)func_e,
        .data = data_str,
    };

    char *root_s;
    get_json_str(&to_resp, &dashboard_data_tmpl_desc, &root_s);

    buffer_t *to_send = init_buffer_unptr();
    CLONE_TO_CHUNK(*to_send, root_s, strlen(root_s));

    // log_fatal("%s %d", root_s, strlen(root_s));
    if (!dashboad_obj->net_ctx.send_handler) {
        log_error("No valid dashboard sending handler");
        return LD_ERR_NULL;
    }
    if (dashboad_obj->net_ctx.send_handler(dashboad_obj->conn, to_send, NULL, NULL) != LD_OK) {
        log_error("Send Dashboard data Failed!");
        return LD_ERR_INTERNAL;
    }

    free_buffer(to_send);
    free(data_str);
    free(root_s);

    return LD_OK;
}

void dashboard_conn_close(basic_conn_t *bc) {
    if (!bc) return;
    free(bc);
    log_warn("Closing connection!");
}


l_err init_dashboard(dashboard_obj_t *dashboad_obj, recv_handler recv_handler) {
    dashboad_obj->net_ctx = (net_ctx_t){
        .conn_handler = dashboard_conn_connect,
        .recv_handler = recv_handler,
        .close_handler = dashboard_conn_close,
        .send_handler = defalut_send_pkt,
        .epoll_fd = core_epoll_create(0, -1),
    };

    dashboad_obj->conn = client_entity_setup(&dashboad_obj->net_ctx, BACKEND_IP, BACKEND_PORT, 0); //不需要绑定本地端口
    pthread_create(&dashboad_obj->conn_th, NULL, net_setup, &dashboad_obj->net_ctx);

    pthread_detach(dashboad_obj->conn_th);
    return LD_OK;
}
