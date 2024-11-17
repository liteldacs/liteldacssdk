//
// Created by jiaxv on 23-8-4.
//

#include "ld_config.h"


config_t config = {
        .port = 8081,
        .debug = FALSE,
        .timeout = 30,
        .worker = 4,
        .ip_ver = IPVERSION_4,
        .init_fl_freq = 960.0,
        .log_dir = "../../log",
        .use_http = FALSE,
        .auto_auth = TRUE,
        .UA = 0,
};


int parse_config(char *yaml_path) {
    init_config();
    parser(yaml_path); /* Parse the path */
    /* Check if requested coil loops number is consistent with config file  */
    /* parsed_loops( &nlp, argv, &coil );*/
    return EXIT_SUCCESS;
}

void init_config() {
}

void realloc_coil() {
}

void parser(char *yaml_path) {
    int i = 0;
    /* Open file & declare libyaml types */
    FILE *fp = fopen(yaml_path, "r");
    yaml_parser_t parser;
    yaml_event_t event;

    bool seq_status = 0; /* IN or OUT of sequence index, init to OUT, currently not needed */
    unsigned int map_seq = 0; /* Index of mapping inside sequence, currently not needed  */
    init_prs(fp, &parser); /* Initiliaze parser & open file */

    do {
        parse_next(&parser, &event); /* Parse new event */
        /* Decide what to do with each event */
        event_switch(&seq_status, &map_seq, &parser, &event, fp);
        if (event.type != YAML_STREAM_END_EVENT) {
            yaml_event_delete(&event);
        }
        i++;
    } while (event.type != YAML_STREAM_END_EVENT);

    clean_prs(fp, &parser, &event); /* clean parser & close file */
}

void event_switch(bool *seq_status, unsigned int *map_seq, yaml_parser_t *parser, yaml_event_t *event, FILE *fp) {
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
            (*seq_status) = TRUE;
            break;
        case YAML_SEQUENCE_END_EVENT:
            (*seq_status) = FALSE;
            break;
        case YAML_MAPPING_START_EVENT:
            if (*seq_status == 1) {
                (*map_seq)++;
            }
            break;
        case YAML_MAPPING_END_EVENT:
            break;
        case YAML_ALIAS_EVENT:
            printf(" ERROR: Got alias (anchor %s)\n",
                   event->data.alias.anchor);
            exit(EXIT_FAILURE);
        case YAML_SCALAR_EVENT:
            to_data(seq_status, map_seq, parser, event, fp);
            break;
        case YAML_NO_EVENT:
            puts(" ERROR: No event!");
            exit(EXIT_FAILURE);
    }
}

void to_data(bool *seq_status, unsigned int *map_seq, yaml_parser_t *parser, yaml_event_t *event, FILE *fp) {
    char *buf = (char *) event->data.scalar.value;

    /* Dictionary */
    char *role = "role";
    char *ua = "UA";
    char *port = "port";
    char *debug = "debug";
    char *init_fl_freq = "init_fl_freq";
    char *init_rl_freq = "init_rl_freq";
    char *addr = "addr";
    char *sgw_addr = "sgw_addr";
    char *sgw_port = "sgw_port";

    char *http_tag = "http";
    char *http_port = "http_port";
    char *auto_auth = "auto_auth";

    if (!strcmp(buf, role)) {
        yaml_event_delete(event);
        parse_next(parser, event);
        char *role_str = (char *) event->data.scalar.value;
        if (!strcmp(role_str, "as")) {
            config.role = LD_AS;
        } else if (!strcmp(role_str, "gs")) {
            config.role = LD_GS;
        } else if (!strcmp(role_str, "gsc")) {
            config.role = LD_GSC;
        }  else if (!strcmp(role_str, "sgw")) {
            config.role = LD_SGW;
        } else {
            log_fatal("Bad role");
            clean_prs(fp, parser, event);
            exit(EXIT_FAILURE);
        }
    } else if (!strcmp(buf, ua)) {
        yaml_event_delete(event);
        parse_next(parser, event);
        config.UA = atoi((char *) event->data.scalar.value);
    } else if (!strcmp(buf, port)) {
        yaml_event_delete(event);
        parse_next(parser, event);
        config.port = atoi((char *) event->data.scalar.value);
    } else if (!strcmp(buf, addr)) {
        yaml_event_delete(event);
        parse_next(parser, event);
        {
            zero(config.addr);
            strcpy(config.addr, (char *)event->data.scalar.value);
        }
        //config.addr = (char *)event->data.scalar.value;
    }  else if (!strcmp(buf, sgw_port)) {
        yaml_event_delete(event);
        parse_next(parser, event);
        config.sgw_port = atoi((char *) event->data.scalar.value);
    } else if (!strcmp(buf, sgw_addr)) {
        yaml_event_delete(event);
        parse_next(parser, event);
        {
            zero(config.sgw_addr);
            strcpy(config.sgw_addr, (char *)event->data.scalar.value);
        }
        //config.addr = (char *)event->data.scalar.value;
    } else if (!strcmp(buf, http_port)) {
        yaml_event_delete(event);
        parse_next(parser, event);
        config.http_port = atoi((char *) event->data.scalar.value);
    } else if (!strcmp(buf, auto_auth)) {
        yaml_event_delete(event);
        parse_next(parser, event);
        config.auto_auth = atoi((char *) event->data.scalar.value) == 1 ? TRUE : FALSE;
    } else if (!strcmp(buf, debug)) {
        yaml_event_delete(event);
        parse_next(parser, event);
        config.debug = atoi((char *) event->data.scalar.value);
    } else if (!strcmp(buf, init_fl_freq)) {
        yaml_event_delete(event);
        parse_next(parser, event);
        config.init_fl_freq = atof((char *) event->data.scalar.value);
    } else if (!strcmp(buf, init_rl_freq)) {
        yaml_event_delete(event);
        parse_next(parser, event);
        config.init_rl_freq = atof((char *) event->data.scalar.value);
    } else if (!strcmp(buf, http_tag)) {
        yaml_event_delete(event);
        parse_next(parser, event);
    } else {
        printf("\n -ERROR: Unknow variable in config file: %s\n", buf);
        clean_prs(fp, parser, event);
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

