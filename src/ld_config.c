//
// Created by jiaxv on 23-8-4.
//

#include <pwd.h>
#include "ld_config.h"

typedef struct {
    char *current_key;    // 当前正在解析的键名
    bool seq_is_peers;
    bool seq_is_direct_gs;
} parse_context;

static void to_data(config_t *config, parse_context *state, yaml_parser_t *parser, yaml_event_t *event, FILE *fp) ;
static void event_switch(config_t *config, parse_context *ctx, yaml_parser_t *parser, yaml_event_t *event, FILE *fp);

static void init_config(config_t *config) {
    // zero(config);
    config->peers = calloc(10, sizeof(void *));
    config->peer_count = 0;
}


int parse_config(config_t *config, char *yaml_path) {
    init_config(config);
    parser(config, yaml_path); /* Parse the path */
    /* Check if requested coil loops number is consistent with config file  */
    /* parsed_loops( &nlp, argv, &coil );*/
    return EXIT_SUCCESS;
}

void realloc_coil() {
}

void parser(config_t *config, char *yaml_path) {
    int i = 0;
    /* Open file & declare libyaml types */
    FILE *fp = fopen(yaml_path, "r");
    yaml_parser_t parser;
    yaml_event_t event;
    parse_context state = {0};

    init_prs(fp, &parser); /* Initiliaze parser & open file */

    do {
        parse_next(&parser, &event); /* Parse new event */
        /* Decide what to do with each event */
        event_switch(config, &state, &parser, &event, fp);
        if (event.type != YAML_STREAM_END_EVENT) {
            yaml_event_delete(&event);
        }
        i++;
    } while (event.type != YAML_STREAM_END_EVENT);

    clean_prs(fp, &parser, &event); /* clean parser & close file */
}

static void handle_key(config_t *config, parse_context *ctx, yaml_event_t *event) {
    char *key = (char *)event->data.scalar.value;
    strncpy(ctx->current_key, key, sizeof(ctx->current_key) - 1);
    ctx->current_key[sizeof(ctx->current_key) - 1] = '\0';
}

void handle_value(config_t *config, parse_context *ctx, yaml_parser_t *parser, yaml_event_t *event, FILE *fp) {
    char *value = (char *)event->data.scalar.value;

    if (!strcmp(ctx->current_key, "role")) {
        char *role_str = (char *) event->data.scalar.value;
        if (!strcmp(role_str, "as")) {
            config->role = LD_AS;
        } else if (!strcmp(role_str, "gs")) {
            config->role = LD_GS;
        } else if (!strcmp(role_str, "gsc")) {
            config->role = LD_GSC;
        }  else if (!strcmp(role_str, "sgw")) {
            config->role = LD_SGW;
        }  else if (!strcmp(role_str, "attack")) {
            config->role = LD_ATTACK;
        } else {
            log_fatal("Bad role");
            clean_prs(fp, parser, event);
            exit(EXIT_FAILURE);
        }
    }else if (!strcmp(ctx->current_key, "UA")) {
        config->UA = atoi((char *) event->data.scalar.value);
    } else if (!strcmp(ctx->current_key, "GS-SAC")) {
        config->GS_SAC = atoi((char *) event->data.scalar.value);
    } else if (!strcmp(ctx->current_key, "AS-SAC")) {
        config->AS_SAC = atoi((char *) event->data.scalar.value);
    } else if (!strcmp(ctx->current_key, "port")) {
        config->port = atoi((char *) event->data.scalar.value);
    } else if (!strcmp(ctx->current_key, "ipv6_address")) {
        {
            zero(config->addr);
            strcpy(config->addr, (char *) event->data.scalar.value);
        }
    }  else if (!strcmp(ctx->current_key, "gsnf_local_port")) {
        config->gsnf_local_port = atoi((char *) event->data.scalar.value);
    }  else if (!strcmp(ctx->current_key, "gsnf_remote_port")) {
        config->gsnf_remote_port = atoi((char *) event->data.scalar.value);
    }   else if (!strcmp(ctx->current_key, "peer_server_port")) {
        config->peer_server_port = atoi((char *) event->data.scalar.value);
    } else if (!strcmp(ctx->current_key, "gsnf_addr")) {
        {
            zero(config->gsnf_addr);
            strcpy(config->gsnf_addr, (char *)event->data.scalar.value);
        }
    }  else if (!strcmp(ctx->current_key, "gsnf_addr_v6")) {
        {
            zero(config->gsnf_addr_v6);
            strcpy(config->gsnf_addr_v6, (char *)event->data.scalar.value);
        }
    } else if (!strcmp(ctx->current_key, "http_port")) {
        config->http_port = atoi((char *) event->data.scalar.value);
    } else if (!strcmp(ctx->current_key, "auto_auth")) {
        config->auto_auth = atoi((char *) event->data.scalar.value) == 1 ? TRUE : FALSE;
    } else if (!strcmp(ctx->current_key, "debug")) {
        config->debug = atoi((char *) event->data.scalar.value);
    } else if (!strcmp(ctx->current_key, "init_fl_freq")) {
        config->init_fl_freq = atof((char *) event->data.scalar.value);
    } else if (!strcmp(ctx->current_key, "init_rl_freq")) {
        config->init_rl_freq = atof((char *) event->data.scalar.value);
    } else if (!strcmp(ctx->current_key, "peer_addr")) {
        if (ctx->seq_is_peers){
            strcpy(config->peers[config->peer_count]->peer_addr, (char *)event->data.scalar.value);
        }
    } else if (!strcmp(ctx->current_key, "peer_SAC")) {
        if (ctx->seq_is_peers) {
            config->peers[config->peer_count]->peer_SAC = atoi((char *) event->data.scalar.value);
        }
    } else if (!strcmp(ctx->current_key, "peer_port")) {
        if (ctx->seq_is_peers) {
            config->peers[config->peer_count]->peer_port = atoi((char *) event->data.scalar.value);
        }
    }else {
        printf("\n -ERROR: Unknow variable in config file: %s\n", ctx->current_key);
        clean_prs(fp, parser, event);
        exit(EXIT_FAILURE);
    }
}

static void event_switch(config_t *config, parse_context *ctx, yaml_parser_t *parser, yaml_event_t *event, FILE *fp) {
    switch (event->type) {
        case YAML_STREAM_START_EVENT:
            break;
        case YAML_STREAM_END_EVENT:
            break;
        case YAML_DOCUMENT_START_EVENT:
            break;
        case YAML_DOCUMENT_END_EVENT:
            break;
        case YAML_SEQUENCE_START_EVENT:
            if (!strcmp(ctx->current_key, "peers")) {
                ctx->seq_is_peers = TRUE;
            }
            if (!strcmp(ctx->current_key, "direct-gs")) {
                ctx->seq_is_direct_gs = TRUE;
            }
            break;
        case YAML_SEQUENCE_END_EVENT:
            if (ctx->seq_is_peers)   ctx->seq_is_peers = FALSE;
            if (ctx->seq_is_direct_gs)   ctx->seq_is_direct_gs = FALSE;
            break;
        case YAML_MAPPING_START_EVENT:
            if (ctx->seq_is_peers) {
                config->peers[config->peer_count] = calloc(1, sizeof(peer_gs_t));
            }
            ctx->current_key = NULL;
            break;
        case YAML_MAPPING_END_EVENT:
            if (ctx->seq_is_peers) {
                config->peer_count++;
            }
            break;
        case YAML_ALIAS_EVENT:
            printf(" ERROR: Got alias (anchor %s)\n",  event->data.alias.anchor);
            exit(EXIT_FAILURE);
        case YAML_SCALAR_EVENT: {
            char* value = (char*)event->data.scalar.value;
            if (ctx->current_key) {
                handle_value(config, ctx, parser, event, fp);
                ctx->current_key = NULL;
            }else {
                ctx->current_key = strdup(value);
            }
            break;
        }
        case YAML_NO_EVENT:
            puts(" ERROR: No event!");
            exit(EXIT_FAILURE);
    }
}

void parse_next(yaml_parser_t *parser, yaml_event_t *event) {
    /* Parse next scalar. if wrong exit with error */
    if (!yaml_parser_parse(parser, event)) {
        printf("Parser error %d\n", parser->error);
        exit(EXIT_FAILURE);
    }
}

void init_prs(FILE *fp, yaml_parser_t *parser) {
    /* Parser initilization */
    if (!yaml_parser_initialize(parser)) {
        fputs("Failed to initialize parser!\n", stderr);
    }

    if (fp == NULL) {
        fputs("Failed to open file!\n", stderr);
    }

    yaml_parser_set_input_file(parser, fp);

}

void clean_prs(FILE *fp, yaml_parser_t *parser, yaml_event_t *event) {
    yaml_event_delete(event); /* Delete event */
    yaml_parser_delete(parser); /* Delete parser */
    fclose(fp); /* Close file */
}

char *get_home_dir() {
    uid_t uid = getuid();
    struct passwd pwd;
    struct passwd *result;
    char *home_dir = calloc(1024, sizeof(char ));
    char buf[1024] = {0}; // 适当调整缓冲区大小

    int ret = getpwuid_r(uid, &pwd, buf, 1024, &result);
    if (ret != 0 || result == NULL) {
        perror("getpwuid_r");
        return NULL;
    }

    memcpy(home_dir, pwd.pw_dir, strlen(pwd.pw_dir));
    return home_dir;
}