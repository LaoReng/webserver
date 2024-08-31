#include "Channel.h"
#include <stdlib.h>


Channel::Channel(int fd, FDEvent events, handleFunc readFunc, handleFunc writeFunc, handleFunc destroyFunc, void* arg)
{
	m_fd = fd;
	m_events = (int)events;
	readCallBack = readFunc;
	writeCallBack = writeFunc;
	destroyCallBack = destroyFunc;
	m_arg = arg;
}

void Channel::writeEventEnable(bool flag)
{
	if (flag)//添加检测写事件
	{
		//m_events |= (int)FDEvent::WriteEvent;
		m_events |= static_cast<int>(FDEvent::WriteEvent);
	}
	else//不需要检测写事件
	{
		m_events = m_events & ~(int)FDEvent::WriteEvent;
	}
}

bool Channel::isWriteEventEnable()
{
	return m_events & (int)FDEvent::WriteEvent;
}
