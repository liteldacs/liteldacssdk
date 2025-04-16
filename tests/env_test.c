//
// Created by root on 4/16/25.
//
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include "ld_config.h"

int main() {
    printf("Home directory (via getpwuid_r): %s\n", get_home_dir());
    return 0;
}