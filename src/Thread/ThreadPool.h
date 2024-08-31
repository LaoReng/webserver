#pragma once
#include "EventLoop.h"
#include <stdbool.h>
#include <vector>
#include "WorkerThread.h"
using namespace std;

//定义线程池 只有一个，主线程拥有
class ThreadPool
{
public:
	ThreadPool(EventLoop* mainLoop, int count);
	~ThreadPool();
	//启动线程池
	void run();
	//取出线程池中某个子线程的反应堆实例
	EventLoop* takeWorkerEventLoop();
private:
	//主线程的反应堆模型
	EventLoop* m_mainLoop;
	bool m_isStart;
	int m_threadNum;
	vector<WorkerThread*> m_workerThreads;
	int m_index;
};



/*
* 线程池操作：上述三个函数都是在主线程中调用，并且依次调用，首先Init函数得到线程池的实例
* 调用Run函数把线程池启动，即启动线程池中的子线程
* 启动之后便可以从子线程中取出一个并进一步得到该子线程的eventloop实例(反应堆)
* 最终得到一个eventloop实例返回给调用者，调用者用这个eventloop往任务队列里添加任务
* 根据任务队列里的任务类型处理任务（ADD,DELETE,MODIFY）并修改选用的监听函数的检测集合(epoll_wait等)
* 下一步反应堆模型就通过epoll_wait函数检测集合中有没有激活的文件描述符。
* 如果有激活的文件描述符，通过该文件描述符以及channelmap找到该文件描述符对应的channel
* 再根据激活的事件，调用对应的回调函数，事件处理完毕。
*/
