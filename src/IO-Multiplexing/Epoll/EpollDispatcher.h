#pragma once
#include "Channel.h"
#include <iostream>
#include "EventLoop.h"
#include "Dispatcher.h"
#include <sys/epoll.h>
#include <string>

using namespace std;
struct EventLoop;
class EpollDispatcher :public Dispatcher
{
public:
	EpollDispatcher(EventLoop* evLoop);
	~EpollDispatcher();
	//添加
	int add() override;//C++11 编译器检测错误
	//删除
	int remove()override;
	//修改
	int modify()override;
	//事件监测 有没有事件被触发
	int dispatcher(int timeout = 2)override;//单位：s
private:
	int epollCtl(int op);
private:
	int m_epfd;//epoll树的根结点
	epoll_event* m_events;
	const int m_maxNode = 520;
};