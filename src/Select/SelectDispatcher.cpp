#include "Dispatcher.h"
#include <sys/select.h>
#include <stdlib.h>
#include <stdio.h>
#include "SelectDispatcher.h"


SelectDispatcher::SelectDispatcher(EventLoop* evLoop):Dispatcher(evLoop)
{
	FD_ZERO(&m_readSet);
	FD_ZERO(&m_writeSet);
	m_name = "Select";
}

SelectDispatcher::~SelectDispatcher()
{
}

int SelectDispatcher::add()
{
	if (m_channel->getSocket() >= m_maxSize)
	{
		return -1;
	}
	setFdSet();
	return 0;
}

int SelectDispatcher::remove()
{
	clearFdSet();
	//通过channel释放对应的tcpconnection资源
	m_channel->destroyCallBack(const_cast<void*>(m_channel->getArg()));
	return 0;
}

int SelectDispatcher::modify()
{
	if (m_channel->getEvent() & (int)FDEvent::ReadEvent)
	{
		FD_SET(m_channel->getSocket(), &m_readSet);
		FD_CLR(m_channel->getSocket(), &m_writeSet);
	}
	if (m_channel->getEvent() & (int)FDEvent::WriteEvent)
	{
		FD_SET(m_channel->getSocket(), &m_writeSet);
		FD_CLR(m_channel->getSocket(), &m_readSet);
	}
	return 0;
}

int SelectDispatcher::dispatcher(int timeout)
{
	struct timeval val;
	val.tv_sec = timeout;
	val.tv_usec = 0;
	fd_set rdtmp = m_readSet;
	fd_set wrtmp = m_writeSet;

	int count = select(m_maxSize, &rdtmp, &wrtmp, NULL, &val);
	if (count == -1)
	{
		perror("select");
		exit(0);
	}
	for (int i = 0; i < m_maxSize; ++i)
	{
		if (FD_ISSET(i, &rdtmp))//文件描述符的读事件 channel里的回调函数
		{
			m_evLoop->eventActivate(i, (int)FDEvent::ReadEvent);
		}
		if (FD_ISSET(i, &wrtmp))//文件描述符的写事件 channel里的回调函数
		{
			m_evLoop->eventActivate(i, (int)FDEvent::ReadEvent);
		}
	}
	return 0;
}

void SelectDispatcher::setFdSet()
{
	if (m_channel->getEvent() & (int) FDEvent::ReadEvent)
	{
		FD_SET(m_channel->getSocket(), &m_readSet);
	}
	if (m_channel->getEvent() & (int)FDEvent::WriteEvent)
	{
		FD_SET(m_channel->getSocket(), &m_writeSet);
	}
}

void SelectDispatcher::clearFdSet()
{
	if (m_channel->getEvent() & (int)FDEvent::ReadEvent)
	{
		FD_CLR(m_channel->getSocket(), &m_readSet);
	}
	if (m_channel->getEvent() & (int)FDEvent::WriteEvent)
	{
		FD_CLR(m_channel->getSocket(), &m_writeSet);
	}
}
