#include <stdio.h>
#include <Server.h>

int main()
{
    printf("Linux向你问好！\n");
    // 初始化用于监听的套接字
    int lfd = initListenFd(9668);
    // 启动服务器程序
    
    return 0;
}