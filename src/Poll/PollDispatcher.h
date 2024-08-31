#pragma once
#include "Channel.h"
#include <iostream>
#include "EventLoop.h"
#include "Dispatcher.h"
#include <poll.h>
#include <string>

using namespace std;
class PollDispatcher :public Dispatcher
{
public:
	PollDispatcher(EventLoop* evLoop);
	~PollDispatcher();
	//添加
	int add() override;//C++11 编译器检测错误
	//删除
	int remove()override;
	//修改
	int modify()override;
	//事件监测 有没有事件被触发
	int dispatcher(int timeout = 2)override;//单位：s

private:
	int m_maxfd;
	struct pollfd* m_fds;
	const int m_maxNode = 1024;
};