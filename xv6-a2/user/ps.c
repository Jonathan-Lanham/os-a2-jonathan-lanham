#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/pinfo.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    struct pinfo table[NPROC];
    int n = getpinfo(table, NPROC);

    if (n < 0) {
        printf("getpinfo failed\n");
        exit(1);
    }

    printf("PID\tSTATE\tLVL\tTICKS\tNSCHED\tCTIME\tNAME\n");
    for (int i = 0; i < n; i++) {
        printf("%d\t%d\t%d\t%d\t%d\t%d\t%s\n",
               table[i].pid,
               table[i].state,
               table[i].level,
               table[i].runticks,
               table[i].nsched,
               table[i].ctime,
               table[i].name);
    }

    exit(0);
}