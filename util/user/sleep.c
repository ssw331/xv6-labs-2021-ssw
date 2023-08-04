#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main (int argc, char *argv[])
{
    // check if there is more or less than 2 arguments in the shell
    // 检查是否多于两个参数
    if(argc != 2)
    {
        fprintf(2, "Arguments missing!\n");
        exit(1);
    }
    int count = atoi(argv[1]);
    sleep(count);
    exit(0);
}