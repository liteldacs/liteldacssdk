//
// Created by jiaxv on 23-8-4.
//

#ifndef TEST_CLIENT_CONFIG_H
#define TEST_CLIENT_CONFIG_H

#include <linux/limits.h>
#include <yaml.h>
#include "ld_log.h"


typedef struct peer_gs_s {
    char peer_addr[128];
    int peer_port;
    uint32_t peer_SAC;
    pthread_t th;
} peer_gs_t;

typedef struct config_s {
    /* general configurations */
    bool debug;
    uint32_t timeout; /* connection_s expired time */
    uint32_t worker; /* worker num */
    char config_path[PATH_MAX];

    /* basic */
    ldacs_roles role;
    double init_fl_freq;
    double init_rl_freq;
    bool is_merged;
    bool is_beihang;
    bool is_e304;

    /* net */
    uint8_t ip_ver;
    char addr[16];
    uint16_t port; /* listen port */
    char gsnf_addr[16];
    char gsnf_addr_v6[128];
    uint16_t gsnf_local_port;
    uint16_t gsnf_remote_port;

    uint16_t peer_server_port;
    peer_gs_t **peers;
    size_t peer_count;


    /* http */
    bool use_http;
    uint16_t http_port; /* listen port */
    bool auto_auth;


    /* role tags */
    uint32_t UA;
    uint8_t ua_gsc; /* temporary, currently for gs */
    uint16_t GS_SAC;

    /* security configurations */
    uint8_t sec_mac_len;
    uint8_t sec_auth_id;
    uint8_t sec_enc_id;
    size_t kdf_len;

    /* log */
    char log_dir[PATH_MAX];
} config_t;

extern config_t config;

/* Prototypes --------------------------------------------------------- */

int parse_config(config_t *config, char *yaml_path) ;

void realloc_coil();

/* Global parser */
void parser(config_t *config, char *yaml_path);

/* Parser utilities */
void init_prs(FILE *fp, yaml_parser_t *parser);

void parse_next(yaml_parser_t *parser, yaml_event_t *event);

void clean_prs(FILE *fp, yaml_parser_t *parser, yaml_event_t *event);



char * get_home_dir();


#endif //TEST_CLIENT_CONFIG_H
