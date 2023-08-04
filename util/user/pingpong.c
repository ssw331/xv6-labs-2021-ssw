#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

// target is to send and receive
int main (int argc, char *argv[])
{
    int pToc[2], cTop[2];
    char msg[1] = {'b'};
    char buf[1];
    if(pipe(pToc) < 0)
    {
        fprintf(2, "Building pipe failed\n");
        exit(1);
    }
    int pid;
    pid = fork();
    if(pid > 0)
    {
        close(pToc[0]);
        write(pToc[1], msg, 1);
        close(pToc[1]);
    }
    else if(pid == 0)
    {
        close(pToc[1]);
        read(pToc[0], buf, 1);
        fprintf(2, "%d: received ping\n", getpid());
        close(pToc[0]);
    }
    if(pipe(cTop) < 0)
    {
        fprintf(2, "Building pipe failed\n");
        exit(1);
    }
    if(pid > 0)
    {
        wait(0);
        close(pToc[1]);
        read(pToc[0], buf, 1);
        fprintf(2, "%d: received pong\n", getpid());
        close(pToc[0]);
        exit(0);
    }
    else if(pid == 0)
    {
        close(pToc[0]);
        write(pToc[1], msg, 1);
        close(pToc[1]);
        exit(0);
    }
    exit(0);
}
