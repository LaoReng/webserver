#include "TcpServer.h"
#include "TcpConnection.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include "Log.h"

int TcpServer::acceptConnection(void *arg)
{
    TcpServer *server = static_cast<TcpServer *>(arg);
    // 和客户端建立连接
    int cfd = accept(server->m_lfd, NULL, NULL);
    // 从线程池中取出一个子线程的反应堆实例，处理这个cfd客户端
    EventLoop *evLoop = server->m_threadPool->takeWorkerEventLoop();
    // 将cfd放到TcpConnection中处理
    new TcpConnection(cfd, evLoop);  // 这块是主线程触发的
    return 0;
}

TcpServer::TcpServer(unsigned short port, int threadNum)
{
    m_port = port;
    setListen();
    m_mainLoop = new EventLoop();
    m_threadNum = threadNum;
    m_threadPool = new ThreadPool(m_mainLoop, threadNum);
}

TcpServer::~TcpServer()
{
}

void TcpServer::setListen()
{
    // 1. 创建监听的fd
    m_lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_lfd == -1)
    {
        perror("socket");
        return;
    }
    // 2. 设置端口复用
    int opt = 1;
    int ret = setsockopt(m_lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof opt);
    if (ret == -1)
    {
        perror("setsockopt");
        return;
    }
    // 3. 绑定
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_port);
    addr.sin_addr.s_addr = INADDR_ANY;
    ret = bind(m_lfd, (struct sockaddr *)&addr, sizeof addr);
    if (ret == -1)
    {
        perror("bind");
        return;
    }
    // 4. 设置监听
    ret = listen(m_lfd, 128);
    if (ret == -1)
    {
        perror("listen");
        return;
    }
}

#include <unistd.h>
#include <sys/types.h>

void TcpServer::run()
{

    // 启动线程池
    m_threadPool->run();

    Debug("<%d>线程池启动了...", gettid());
    // 添加检测的任务：首先将用于连接的服务器端口用mainloop监听
    // 初始化一个channel实例
    Channel *channel = new Channel(m_lfd, FDEvent::ReadEvent, acceptConnection,
                                   nullptr, nullptr, this);
    Debug("channel监听初始化成功...");
    m_mainLoop->addTask(channel, ElemType::ADD);
    // 启动反应堆模型
    m_mainLoop->run(); //
    Debug("服务器程序已经启动了...");
}
