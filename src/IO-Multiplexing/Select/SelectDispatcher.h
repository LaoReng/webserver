#pragma once
#include "Channel.h"
#include <iostream>
#include "EventLoop.h"
#include "Dispatcher.h"
#include <sys/epoll.h>
#include <string>

using namespace std;
struct EventLoop;
class SelectDispatcher :public Dispatcher
{
public:
	SelectDispatcher(EventLoop* evLoop);
	~SelectDispatcher();
	//添加
	int add() override;//C++11 编译器检测错误
	//删除
	int remove()override;
	//修改
	int modify()override;
	//事件监测 有没有事件被触发
	int dispatcher(int timeout = 2)override;//单位：s
private:
	void setFdSet();
	void clearFdSet();
private:
	fd_set m_readSet;
	fd_set m_writeSet;
	const int m_maxSize = 1024;
};