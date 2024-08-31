#include "WorkerThread.h"
#include "Log.h"
#include <stdio.h>

WorkerThread::WorkerThread(int index)
{
	m_evLoop = nullptr;
	m_thread = nullptr;
	m_threadID = thread::id();//这里原本是0，官方源码也是0，修改为index
	m_name = "SubThread-" + to_string(index);
}

WorkerThread::~WorkerThread()
{
	if (m_thread != nullptr)
	{
		delete m_thread;
	}
}

void WorkerThread::run()//主线程调用的
{
	//创建子线程
	m_thread = new thread(&WorkerThread::running, this);
	//阻塞主线程，等待子线程初始化完毕，否则会直接结束
	unique_lock<mutex> locker(m_mutex);
	while (m_evLoop == nullptr)
	{
		m_cond.wait(locker);
	}
}

//子线程的回调函数
void* WorkerThread::running()
{
	// 每个子线程启动时，都会自己创建一个自己的EventLoop
	m_mutex.lock();
	m_evLoop = new EventLoop(m_name);
	m_mutex.unlock();
	m_cond.notify_one();
	m_evLoop->run();
	return NULL;
}
