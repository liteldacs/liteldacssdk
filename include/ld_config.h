//
// Created by jiaxv on 23-8-4.
//

#ifndef TEST_CLIENT_CONFIG_H
#define TEST_CLIENT_CONFIG_H

#include "../global/ldacs_sim.h"
#include "ld_log.h"

#define UA_LEN 50
#define MAX_PATH_LEN 4096
#define KEY_SIZE (4)

#define UA_AS_LEN (8)
#define UA_GS_LEN (4)
#define UA_GSC_LEN (4)

typedef enum {
    LD_AS = 0x0,
    LD_GS = 0x1,
    LD_GSC = 0x2,
    LD_SGW = 0x3,
    LD_ATTACK = 0x4,
} ldacs_roles;

static const char *roles_str[] = {
    "LD_AS",
    "LD_GS",
    "LD_GSC",
    "LD_SGW",
    "LD_ATTACK",
};

typedef enum {
    LD_UDP_CLIENT,
    LD_UDP_SERVER,
    LD_TCP_CLIENT,
    LD_TCP_SERVER,
} sock_roles;


typedef struct config_s {
    /* general configurations */
    bool debug;
    uint32_t timeout; /* connection_s expired time */
    uint32_t worker; /* worker num */

    /* basic */
    ldacs_roles role;
    double init_fl_freq;
    double init_rl_freq;
    bool is_merged;

    /* net */
    uint8_t ip_ver;
    char addr[16];
    uint16_t port; /* listen port */
    char gsc_addr[16];
    uint16_t gsc_port;
    char gsnf_addr[16];
    char gsnf_addr_v6[16];
    uint16_t gsnf_port;

    /* http */
    bool use_http;
    uint16_t http_port; /* listen port */
    bool auto_auth;


    /* role tags */
    uint32_t UA;
    uint8_t ua_gsc; /* temporary, currently for gs */

    /* security configurations */
    uint8_t sec_mac_len;
    uint8_t sec_auth_id;
    uint8_t sec_enc_id;
    size_t kdf_len;

    /* log */
    char log_dir[1024];
} config_t;

extern config_t config;

/* Prototypes --------------------------------------------------------- */

int parse_config(config_t *config, char *yaml_path) ;

/* Struct initialization */
void init_config();

void realloc_coil();

/* Global parser */
void parser(config_t *config, char *yaml_path);

/* Parser utilities */
void init_prs(FILE *fp, yaml_parser_t *parser);

void parse_next(yaml_parser_t *parser, yaml_event_t *event);

void clean_prs(FILE *fp, yaml_parser_t *parser, yaml_event_t *event);

/* Parser actions */
void event_switch(config_t *config, bool *seq_status, unsigned int *map_seq, yaml_parser_t *parser, yaml_event_t *event, FILE *fp);

void to_data(config_t *config, bool *seq_status, unsigned int *map_seq,
             yaml_parser_t *parser, yaml_event_t *event, FILE *fp);



#endif //TEST_CLIENT_CONFIG_H
