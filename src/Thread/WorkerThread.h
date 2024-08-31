#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include "EventLoop.h"
using namespace std;

//子线程结构体
class WorkerThread
{
public:
	WorkerThread(int index);//index指示这个线程是当前线程的第几个线程，用来命名
	~WorkerThread();
	//启动
	void run();
	inline EventLoop* getEventLoop() {
		return m_evLoop;
	}
private:
	void* running();
private:
	thread* m_thread;//保存线程的实例
	thread::id m_threadID;//ID
	string m_name;
	mutex m_mutex;//互斥锁
	condition_variable m_cond;//条件变量
	EventLoop* m_evLoop; //反应堆模型
};

