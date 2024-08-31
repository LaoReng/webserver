#include "EventLoop.h"
#include <assert.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "EpollDispatcher.h"
#include "PollDispatcher.h"
#include "SelectDispatcher.h"

EventLoop::EventLoop() 
    : EventLoop(string()) // c++11委托构造函数
{
}

EventLoop::EventLoop(const string threadName)
{
    printf("%s(%d)<%d>:事件炉构造被调用了\n", __FILE__, __LINE__, gettid());
    // 每个EventLoop里面都会新创建一个epoll
    m_isQuit = true;                    // 默认没有启动
    m_threadID = this_thread::get_id(); // c++11
    m_threadName = threadName == string() ? "MainThread" : threadName;
    m_dispatcher = new EpollDispatcher(this); // 选择epoll函数处理
    // map
    m_channelMap.clear();
    int ret = socketpair(AF_UNIX, SOCK_STREAM, 0, m_socketPair);
    if (ret == -1)
    {
        perror("socketpair");
        exit(0);
    }
    printf("%s(%d)<%d>:管道[0]:%d 管道[1]:%d\n", __FILE__, __LINE__, gettid(), m_socketPair[0], m_socketPair[1]);
#if 0
    //指定规则：evloop->socketPair[0] 发送数据 ，evloop->socketPair[1]接收数据
    //通过sockpair[0]往socketpair[1]中写数据，然后用epoll监听socketpair[1]，可以实现人为唤醒子线程
    Channel* channel = new Channel(m_socketPair[1], FDEvent::ReadEvent, readLocalMessage, nullptr, nullptr, this);
#else
    // 绑定-bind c++11可调用对象绑定器,将函数绑定给对象
    // 使用可调用对象包装器(function)不能直接对类的非静态成员函数进行打包，需要通过绑定器绑定
    // 得到一个可调用对象obj
    // 这里需要绑定this的原因是：成员函数默认是有一个this函数形参的（也就是说需要依赖this指针来调用），还有一个原因是它的参数不符合条件需要让bind将其变得符合
    auto obj = bind(&EventLoop::readMessage, this);
    Channel *channel = new Channel(m_socketPair[1], FDEvent::ReadEvent,
                                   obj, nullptr, nullptr, this);

#endif
    // channel添加到任务队列
    addTask(channel, ElemType::ADD);
}

EventLoop::~EventLoop()
{
}

int EventLoop::run()
{
    m_isQuit = false;
    // 比较线程id是否正常(只能用eventloop自己的线程运行)
    if (m_threadID != this_thread::get_id())
    {
        return -1;
    }
    // 循环进行事件处理
    while (!m_isQuit)
    {
        // 调用回调函数
        m_dispatcher->dispatcher(); // 这个函数对应就是调用三种监听函数的监听过程，如epoll_wait  2指示超时时长，传递evloop函数用来使其可以修改evloop下的监听data内容(data->events)
        processTaskQ();             // 这是从任务队列中取任务并添加到epoll监听中
    }
    return 0;
}

int EventLoop::eventActivate(int fd, int event)
{
    if (fd < 0)
    {
        return -1;
    }
    // 取出channel
    Channel *channel = m_channelMap[fd];
    assert(channel->getSocket() == fd);
    if (event & (int)FDEvent::ReadEvent && channel->readCallBack)
    {
        channel->readCallBack(const_cast<void *>(channel->getArg())); // 调用回调函数
    }
    if (event & (int)FDEvent::WriteEvent && channel->writeCallBack)
    {
        channel->writeCallBack(const_cast<void *>(channel->getArg())); // 调用回调函数
    }
    return 0;
}

int EventLoop::addTask(Channel *channel, ElemType type)
{
    // 加锁，保护共享资源 ,可能被多个线程访问
    m_mutex.lock();
    // 创建新结点
    ChannelElement *node = new ChannelElement;
    node->channel = channel;
    node->type = type;
    m_taskQ.push(node);
    /*
     * 这是往任务队列里添加了一个任务，任务是对某个文件描述符(channel)进行操作(type(ADD,DELETE,MODIFY))
     * 而不是直接对channel中文件描述符的属性进行修改。
     * 因此下一步是取链表中的任务，然后根据任务type对相应的文件描述符就行操作。
     */
    m_mutex.unlock();
    // 处理节点
    /*
    * 细节:
    *   1. 对于链表节点的添加: 可能是当前线程也可能是其他线程(主线程)
           1). 修改fd的事件, 当前子线程发起, 当前子线程处理
    *       2). 添加新的fd, 添加任务节点的操作是由主线程发起的
    *   2. 不能让主线程处理任务队列, 需要由当前的子线程取处理
    */

    /*
    * 每个eventloop都有一个dispatcher，每个dispatcher对应一个epoll，poll或select，
    从而对应一个文件描述符的map以及任务队列，不同eventloop处理任务不能互通，
    因为选择的监听，以及监听的data不同
    */

    if (m_threadID == this_thread::get_id())
    {
        // 自己调用自己反应堆的添加任务函数，则自己处理
        processTaskQ(); // 子线程修改任务队列，子线程自己处理
    }
    else // 其余线程
    {
        // 主线程 -- 告诉子线程处理任务队列中的任务
        //  1. 子线程在工作 2. 子线程被阻塞了:select, poll, epoll 可能需要人为解除阻塞
        /**
         * 这里一定要注意，如果是连接客户端，那么这里是主线程调用的事件炉中的每个event
         * 并且用这个event的管道来通知这个event进行工作
         *
         * 这里为什么要去激活别人的event，因为这时候可能event还阻塞在epoll_wait这里，
         * 这时如果我们不去唤醒的话，有可能这个任务就不会被执行到（为什么是有可能：因为用户设置的epoll_wait的等待时间可能是无限）
         * 这就是我们把管道注册到epoll中的原因，当有任务抛到event时可以快速的激活event
         */
        taskWakeup(); // 其余线程唤醒子线程 然后子线程处理任务，
    }
    return 0;
}

int EventLoop::processTaskQ() // 处理任务队列中的任务
{
    while (!m_taskQ.empty())
    {
        m_mutex.lock(); // 只需要锁住操作任务队列的过程
        ChannelElement *node = m_taskQ.front();
        m_taskQ.pop();
        m_mutex.unlock();
        Channel *channel = node->channel;
        // 取出任务的信息，对channel的文件描述符进行修改
        if (node->type == ElemType::ADD)
        {
            // 添加
            add(channel);
        }
        else if (node->type == ElemType::DELETE)
        {
            // 删除
            remove(channel);
        }
        else if (node->type == ElemType::MODIFY)
        {
            // 修改
            modify(channel);
        }
        delete node;
    }

    return 0;
}

int EventLoop::add(Channel *channel)
{
    // 此处是把任务队列中的任务添加到epoll_wait的检测集合中
    int fd = channel->getSocket();
    // 找到fd对应的数据元素位置，并存储
    if (m_channelMap.find(fd) == m_channelMap.end())
    {
        m_channelMap.insert(make_pair(fd, channel));
        m_dispatcher->setChannel(channel);
        int ret = m_dispatcher->add();
        return ret;
    }
    return -1;
}

int EventLoop::remove(Channel *channel)
{
    int fd = channel->getSocket();
    if (m_channelMap.find(fd) == m_channelMap.end())
    {
        return -1;
    }
    m_dispatcher->setChannel(channel);
    int ret = m_dispatcher->remove();
    return ret;
}

int EventLoop::modify(Channel *channel)
{
    int fd = channel->getSocket();
    if (m_channelMap.find(fd) == m_channelMap.end())
    {
        return -1;
    }
    m_dispatcher->setChannel(channel);
    int ret = m_dispatcher->modify();
    return ret;
}

int EventLoop::freeChannel(Channel *channel)
{
    // 删除channel和fd的对应关系
    auto it = m_channelMap.find(channel->getSocket());
    if (it != m_channelMap.end())
    {
        m_channelMap.erase(it);
        close(channel->getSocket());
        delete channel;
    }
    return 0;
}

int EventLoop::readMessage()
{
    char buf[256];
    read(m_socketPair[1], buf, sizeof(buf)); // 触发读事件，使其解除阻塞
    printf("%s(%d)<%d>:函数被触发 管道[1]:%d\n", __FILE__, __LINE__, gettid(), m_socketPair[1]);
    return 0;
}

void EventLoop::taskWakeup()
{
    const char *msg = "上班！";
    printf("%s(%d)<%d>:我触发的函数 管道[0]:%d\n", __FILE__, __LINE__, gettid(), m_socketPair[0]);
    write(m_socketPair[0], msg, strlen(msg));
}
