#include "ThreadPool.h"
#include <assert.h>
#include <stdlib.h>
#include "Log.h"


ThreadPool::ThreadPool(EventLoop* mainLoop, int count)
{
	m_index = 0;
	m_isStart = false;
	m_mainLoop = mainLoop;
	m_threadNum = count;
	m_workerThreads.clear();
}

ThreadPool::~ThreadPool()
{
	for (auto item : m_workerThreads)
	{
		delete item;
	}
}

void ThreadPool::run()
{
	assert(!m_isStart);
	if (m_mainLoop->getThreadID() != this_thread::get_id())//正常情况下，启动线程池的线程是主线程
	{
		exit(0);
	}
	m_isStart = true;
	if (m_threadNum > 0)
	{
		for (int i = 0; i < m_threadNum; ++i)
		{
			// 这是启动线程池，将每一个线程创建好
			WorkerThread* subThread = new WorkerThread(i);
			subThread->run();
			m_workerThreads.push_back(subThread);
		}
	}
}

/*
* 这个函数肯定是由主线程调用,主线程拥有线程池，主线程遍历线程池
* 从中挑选一个子线程，得到其eventloop反应堆，再把要处理的任务放到这个反应堆模型中
*/
EventLoop* ThreadPool::takeWorkerEventLoop()
{
	assert(m_isStart);
	if (m_mainLoop->getThreadID() != this_thread::get_id())
	{
		exit(0);
	}
	//从线程池中找出一个子线程，并取出反应堆实例
	EventLoop* evLoop = m_mainLoop;//首先默认反应堆是主线程的反应堆，如果线程池里没有子线程，就选用主线程的反应堆模型
	if (m_threadNum > 0)
	{
		evLoop = m_workerThreads[m_index]->getEventLoop();
		m_index = ++m_index % m_threadNum;
	}
	return evLoop;
}
