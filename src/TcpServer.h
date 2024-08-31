#pragma once
#include "EventLoop.h"
#include "ThreadPool.h"


class TcpServer
{
public:
	//初始化
	TcpServer(unsigned short port, int threadNum);
	~TcpServer();
	//初始化监听
	void setListen();
	//启动服务器
	void run();
	static int acceptConnection(void* arg);//设置为静态或者使用可调用函数绑定，才能作为回调函数
private:
	int m_threadNum;
	EventLoop* m_mainLoop;//主线程的反应堆模型
	ThreadPool* m_threadPool;
	int m_lfd;
	unsigned short m_port;
};



/*
* 这一部分什么功能：
* 服务器端首先有一个用于监听的文件描述符以及用来通信的服务器端口，
基于这个监听的文件描述符就可以和客户端建立连接，如果想要Listenfd进行工作，
就必须将其放到一个反应堆模型中，因此，首先要把listenfd进行封装，封装成一个channel类型
然后将其添加到任务队列中，任务是增加监听这个listenfd。
然后让主eventloop反应堆开启，遍历任务队列，取出任务结点，根据任务type对监听的文件描述符进行
ADD,DELETE或MODIFY。当文件描述符对应的读事件触发之后，根据文件描述符以及channelmap，运行其回调函数
处理读事件，即与客户端建立连接
主反应堆建立连接之后，获得与客户端通信的文件描述符，首先要将其封装成一个channel类型，
然后再把其封装到一个tcpconnection中，这个tcpconnection是运行在子线程中，
因此需要主线程访问线程池，从中找出一个子线程(每个子线程都有一个eventloop),把这个eventloop也放到tcpconnection中
因此可以再tcpconnection中，通过channel内封装的用于通信的文件描述符与客户端进行通信
这个文件描述符的事件检测都是通过传过去的eventloop来实现的
*/