#include "kernel/types.h"
#include "kernel/riscv.h"
#include "kernel/sysinfo.h"
#include "user/user.h"

int
main(int argc, char* argv[]) {
    if(argc != 1) {
        fprintf(2, "Too much arguments!\n");
        exit(1);
    }
    struct sysinfo info;  // 这是分配在用户区的
    sysinfo(&info);
    printf("free space: %d\nused process: %d\n", info.freemem, info.nproc);
    exit(0);
}