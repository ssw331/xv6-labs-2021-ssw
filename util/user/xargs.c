#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

#define MAX 512

int main(int argc, char* argv[])
{
    if(argc < 2) {
        fprintf(2, "Argument missing.\n");
        exit(1);
    }
    char* args[MAXARG];
    int index = 0;
    // 保存xargs 的参数
    for(int i = 1; i < argc ; i++) {
        args[index++] = argv[i];
    }

    char buf[MAX] = {0};

    // newLine保存新的参数
    char* newLine;
    newLine = malloc(MAX);
    args[index] = newLine;
    int idx = 0;
    // 0 为shell中的管道符号的读端
    // 读取输出到管道中的数据作为参数给到xargs后的指令
    while(read(0, buf, 1) > 0) {
        // 开始读取管道中的参数
        // 读到换行符即读完管道中的一组数据数据
        if (buf[0] == '\n') {
            // 创建子线程处理参数
            int pid = fork();
            if (pid == 0) {
                exec(argv[1], args);
                exit(0);
            } else
                // 父线程等待子线程结束，防止并行输出错误
                wait(0);
            free(newLine);  // 这里只是给内存上了标签
            newLine = malloc(MAX);  // 在上面释放的内存没有被改变时，这里的分配依然是上面free的空间
            memset(newLine, 0, MAX);  // 所以手动把它重置
            args[index] = newLine;
            idx = 0;
        } else if (buf[0] != '\n') {
            // 将管道的输出作为参数传给args
            newLine[idx] = buf[0];
            fprintf(2, "%s\n", newLine);
            idx++;
        }
    }
    exit(0);
}