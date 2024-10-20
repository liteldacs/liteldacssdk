//
// Created by 邹嘉旭 on 2024/5/13.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[], char *envp[]) {
    //daemon(1, 1);
    //load parameters
    char s_outputdir[100];
    char s_result[20];
    char s_command[strlen(
                       "iftop -nNtP -s 60 2>/dev/null | grep 65536 | cut -d b -f4 | sed 's/ //g' | grep B | sed -e '/Cumulative/d'")
                   + 1];
    double d_sum = 0;

    printf("%s %s %s", argv[0], argv[1], argv[2]);
    sprintf(s_command, "ps aux | grep %s | sed -e '/grep/d' | cut -d ' ' -f1", argv[0]);
    //printf("\n%s\n",s_command);
    FILE *fp = NULL;
    fp = popen(s_command, "r");
    fgets(s_outputdir, sizeof(s_outputdir) - 1, fp);
    memset(s_outputdir, 0, sizeof(s_outputdir));
    fgets(s_outputdir, sizeof(s_outputdir) - 1, fp);
    fclose(fp);
    if (s_outputdir[0]) {
        printf("\nprocess already exists!");
        return 1;
    }
    //printf("%d", argc);	//when you enter 2 paras, it should be 3 now.
    if (argc != 4 && argc != 2) {
        printf("\nwrong parameter numbers!\n");
        return 1;
    }
    //printf("%s\n", argv[1]);
    if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
        printf("\nUsage:[command dir] <second> <port number> <file name>\n");
        return 0;
    }
    if (atoi(argv[1]) > 60 || atoi(argv[1]) <= 0) {
        printf("\nsecond larger than 1 min or less than 1 sec.!\n");
        return 1;
    }
    if (atoi(argv[2]) > 65536 || atoi(argv[2]) <= 0) {
        printf("\nport number should be smaller than 65536 and larger than 0!\n");
        return 1;
    }
    strcpy(s_outputdir, argv[3]);
    //printf("%s %s\n", s_second, s_portnumber);
    //prepare for the shell cmd.
    sprintf(s_command,
            "iftop -nNtP -s %s 2>/dev/null | grep %s | cut -d b -f4 | sed 's/ //g' | grep B | sed -e '/Cumulative/d'",
            argv[1], argv[2]);
    //printf("%s\n", s_command);
    //do iftop
    while (1) {
        //make a daemon.
        fp = NULL;
        fp = popen(s_command, "r");
        if (!fp) {
            printf("\ndoing shell error!\n");
            return 1;
        }
        memset(s_result, 0, sizeof(s_result));
        d_sum = 0;
        //load result and make a sum.
        while (fgets(s_result, sizeof(s_result) - 1, fp)) {
            //printf("%s %d\n", s_result, strlen(s_result));	//when result is 1.76MB, strlen is 7.
            if (s_result[0] == 'B' || s_result[0] == 'K' || s_result[0] == 'M')continue;
            else if (s_result[strlen(s_result) - 3] == 'M') {
                d_sum += atof(s_result);
                continue;
            } else if (s_result[strlen(s_result) - 3] == 'K') {
                d_sum += atof(s_result) / 1000;
                continue;
            } else {
                d_sum += atof(s_result) / 1000000;
                continue;
            }
        }
        pclose(fp);
        //printf("\nbefore open the file:%f\n",d_sum);
        //output sum to a file.
        fp = NULL;
        fp = fopen(s_outputdir, "r");
        //printf("opened the file with ro.\n");
        memset(s_result, 0, sizeof(s_result));
        if (!fp)printf("\nno file exist! trying to create one.\n");
        else {
            fgets(s_result, sizeof(s_result) - 1, fp); //we borrow s_result to load the old sum.
            fclose(fp);
            //printf("closed the file in ro.\n");
        }
        fp = NULL;
        //printf("opened the file in wo\n");
        fp = fopen(s_outputdir, "w");
        //printf("\nold:%fMB\n", d_sum);
        d_sum += atof(s_result);
        //printf("\nnew:%fMB\n", d_sum);
        fprintf(fp, "%fMB\n", d_sum);
        fclose(fp);
        //printf("closed the file.\n");
    }
    return 0;
}
