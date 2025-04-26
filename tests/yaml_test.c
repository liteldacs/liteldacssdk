//
// Created by 邹嘉旭 on 2025/4/26.
//
#include "ld_config.h"


config_t configg = {
};
int main() {
    parse_config(&configg, "../../test.yaml");
}