#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// realize the func dup2
void dup2(int n, int fd[])
{
    close(n);  //close a certain fd. 关闭指定fd
    dup(fd[n]); // duplicate fd[n] to the min file describer. 复制fd
    // close the pipe. 关闭管道
    close(fd[0]);
    close(fd[1]);
}

void prime()
{
    int msg;
    int fd[2];
    if(!(read(0, &msg, sizeof(int)))) // 读取管道中的第一个元素
        exit(0);
    int p = msg;
    fprintf(2, "prime %d\n", p);
    if(pipe(fd) < 0)
    {
        fprintf(2, "Out of Sources!");
        exit(0);
    }
    int pid = fork();
    if(pid == 0)
    {
        dup2(1, fd); // 先将本次递归中的pipe的写端映射到1
        while(read(0, &msg, sizeof(int)))
        {
            if(msg % p != 0)
                write(1, &msg, sizeof(int)); // 此时写给本次递归的pipe
        }
        exit(0);
    }
    else if(pid > 0)
    {
        wait(0);
        dup2(0, fd);
        prime();
        exit(0);
    }
}

int main ()
{
    int fd[2];
    if(pipe(fd) < 0)
    {
        fprintf(2, "Out of Sources!");
        exit(0);
    }
    int pid = fork();
    if(pid == 0)
    {
        dup2(1, fd);
        for(int i = 2; i < 36; i++)
            write(1, &i, sizeof(int));
        exit(0);
    }
    else if(pid > 0)
    {
        wait(0);
        dup2(0, fd);
        prime();
    }
    exit(0);
}
