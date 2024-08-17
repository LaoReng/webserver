#include "Server.h"
#include <stdio.h>
#include <arpa/inet.h>  // 网络编程套接字，里面包含socket.h
#include <sys/epoll.h>  // epollAPI所在头文件
#include <fcntl.h>

int initListenFd(unsigned short port)
{
    // 1. 创建监听的fd
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if(lfd == -1){
        perror("socket");
        return -1;
    }
    // 2. 设置端口复用
    int opt = 1;
    int ret = setsockopt(lfd, SOL_SOCKET,SO_REUSEADDR, &opt,sizeof(opt));
    if(ret == -1){
        perror("setsockopt");
        return -1;
    }
    // 3. 绑定
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    ret = bind(lfd, (sockaddr*)&addr, sizeof(addr));
    if(ret == -1){
        perror("bind");
        return -1;
    }
    // 4. 设置监听
    ret = listen(lfd, 4);  // 内核里面这个最大连接数是128，如果你给的数超过128他也会按128算
    if(ret == -1){
        perror("listen");
        return -1;
    }
    // 返回fd
    return lfd;
}

int epollRun(int lfd)
{
    // 1.创建epoll实例
    int epfd = epoll_create(1);
    if(epfd == -1){
        perror("epoll_create");
        return -1;
    }
    // 2. lfd上树
    struct epoll_event ev;
    ev.data.fd = lfd;
    ev.events = EPOLLIN;
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD,lfd, &ev);
    if(ret == -1){
        perror("epoll_ctl");
        return -1;
    }
    // 3. 检测
    struct epoll_event evs[1024];
    while(1){
        int num = epoll_wait(epfd, evs, sizeof(evs)/sizeof(*evs), -1);
        for(int i=0;i<num;i++){
            int fd = evs[i].data.fd;
            if(fd == lfd) {
                // 建立新连接 accept
                acceptClient(lfd, epfd);
            }
            else {
                // 主要是接收对端的数据
                
            }
        }
    }
    return 0;
}

int acceptClient(int lfd, int epfd)
{
    // 1.  建立连接
    int cfd = accept(lfd, NULL, 0);
    if(cfd == -1){
        perror("accept");
        return -1;
    }
    // 2. 设置文件描述符为非阻塞
    int flag = fcntl(cfd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(cfd, F_SETFL, flag);
    // 3. cfd添加到epoll中
    struct epoll_event ev;
    ev.data.fd = cfd;
    ev.events = EPOLLIN | EPOLLET; // EPOLLET 设置边缘触发
    int ret = epoll_ctl(epfd, EPOLL_CTL_ADD,cfd, &ev);
    if(ret == -1){
        perror("epoll_ctl");
        return -1;
    }
    return 0;
}