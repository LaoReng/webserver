#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "Server.h"

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("./a.out port path\n");
        return -1;
    }
    unsigned short port = atoi(argv[1]);
    // 切换服务器的工作路径
    chdir(argv[2]);
    // 初始化用于监听的套接字
    int lfd = initListenFd(port);
    if (lfd < 0)
    {
        perror("套接字初始化失败!");
        return 0;
    }
    // 启动服务器程序
    epollRun(lfd);
    return 0;
}