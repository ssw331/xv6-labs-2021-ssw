#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

static int flag = 0;

// 引用ls.c
char*
fmtname(char *path)
{
    static char buf[DIRSIZ+1];
    char *p;

    // Find first character after last slash.
    for(p=path+strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    // Return blank-padded name.
    if(strlen(p) >= DIRSIZ)
        return p;
    memmove(buf, p, strlen(p));
    memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
    return buf;
}

void find(char* path, char* file_name)
{
    // 文件名缓冲区与指针
    char buf[512], *p;
    // 文件描述符
    int fd;
    struct dirent de;
    // 文件状态结构体
    struct stat st;

    // ensure the file/directory can be open correctly.
    // 保证文件/目录可以被正常打开
    if((fd = open(path, 0)) < 0)
    {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    // ensure the status of file/dir.
    // 确认文件/目录状态
    if(fstat(fd, &st) < 0)
    {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch(st.type)
    {
        case T_FILE:
            printf("This is no directory.\n");
            exit(3);
            break;

        case T_DIR:
            if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
                printf("find: path too long\n");
                break;
            }
            strcpy(buf, path);
            p = buf+strlen(buf);
            *p++ = '/';  // 在路径最后加上/
            while(read(fd, &de, sizeof(de)) == sizeof(de)){
                if(de.inum == 0)
                    continue;
                memmove(p, de.name, DIRSIZ);  // 从de.name复制DIRSIZ个字符到p中
                // 此时的buf = path + '/' + de.name
                // p -> '/'
                p[DIRSIZ] = 0;  // 设置字符串终止符
                if(stat(buf, &st) < 0){
                    printf("find: cannot stat %s\n", buf);
                    continue;
                }
                if(st.type == T_DIR)
                {
                    if(strcmp(de.name, ".") != 0 && strcmp(de.name, "..") != 0)
                    {
                        find(buf, file_name);  // 深度优先，直到只有文件后回溯
                    }
                }
                else if(st.type == T_FILE)
                {
                    // de.name不带空格
                    if(!strcmp(de.name, file_name))
                    {
                        printf("%s\n", buf);
                        flag = 1;
                    }
                }
            }
            break;
    }
}

int main(int argc, char *argv[])
{
    if(argc < 3)
    {
        fprintf(2, "Argument missing.\n");
        exit(1);
    }
    else if(argc == 3)
    {
        find(argv[1], argv[2]);
        if(!flag)
        {
            fprintf(2, "find: cannot find %s\n", argv[2]);
        }
    }
    else
    {
        fprintf(2, "Too Much Argument.\n");
        exit(2);
    }
    exit(0);
}