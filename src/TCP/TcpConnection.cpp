#include <stdio.h>
#include <stdlib.h>
#include "TcpConnection.h"
#include "HttpRequest.h"
#include "Log.h"


int TcpConnection::processRead(void* arg)
{
	//接收数据
	TcpConnection* conn = static_cast<TcpConnection*>(arg);
	int socket = conn->m_channel->getSocket();
	int count = conn->m_readBuf->socketRead(socket);
	Debug("接收到的http请求数据:%s", conn->m_readBuf->data());
	if (count > 0)
	{
		//接收到了http请求 解析http请求
#ifdef MSG_SEND_AUTO
		//设置写事件，通过触发写事件发送
		conn->m_channel->writeEventEnable(true);
		conn->m_evLoop->addTask(conn->m_channel, ElemType::MODIFY);
#endif // MSG_SEND_AUTO
		//一边解析一边发送
		bool flag = conn->m_request->parseRequest(conn->m_readBuf, 
			conn->m_response, conn->m_writeBuf, socket);
		if (!flag)
		{
			//解析失败
			string errMsg = "Http/1.1 400 Bad Request\r\n\r\n";
			conn->m_writeBuf->appendString(errMsg);
		}
	}
	else
	{
#ifdef MSG_SEND_AUTO
		//断开连接
		conn->m_evLoop->addTask(conn->m_channel, ElemType::DELETE);
#endif // MSG_SEND_AUTO
	}
#ifndef MSG_SEND_AUTO
	// 断开连接
	conn->m_evLoop->addTask(conn->m_channel, ElemType::DELETE);
#endif
	return 0;
}

int TcpConnection::processWrite(void* arg)
{
	//写回调，发送数据
	Debug("开始发送数据了(基于写事件发送)....");//现在用的是边解析边发送，没有用到写事件，所以写回调没有调用过
	TcpConnection* conn = static_cast<TcpConnection*>(arg);
	//发送数据

	int count = conn->m_writeBuf->sendData(conn->m_channel->getSocket());
	if (count > 0)
	{
		//判断数据是否全被发送出去了
		if (conn->m_writeBuf->readableSize() == 0) //这个buf中可读的数据为0，即没有数据了，全部发送出去了
		{
			//发送完了，不再检测写事件--修改channel中保存的事件
			conn->m_channel->writeEventEnable(false);
			//修改dispatcher检测的集合--添加任务结点
			conn->m_evLoop->addTask(conn->m_channel, ElemType::MODIFY);
			//删除这个结点
			conn->m_evLoop->addTask(conn->m_channel, ElemType::DELETE);
		}
	}
	return 0;
}

int TcpConnection::destroy(void* arg)
{
	Debug("连接断开，开始释放资源");
	TcpConnection* conn = static_cast<TcpConnection*>(arg);
	if (conn != nullptr)
	{
		delete conn;
	}
	return 0;
}

TcpConnection::TcpConnection(int fd, EventLoop* evLoop)
{
	m_evLoop = evLoop;
	m_readBuf = new Buffer(10240);
	m_writeBuf = new Buffer(10240);
	//http
	m_request = new HttpRequest;//与readbuf对应，接收到的数据存储到readbuf中，然后从中读数据并解析http请求，解析完放到httprequest中
	m_response = new HttpResponse;//与writebuf对应，回复数据时，通过httpresponse组织回复数据，然后写到writebuf中，然后发送出去
	m_name = "Connection-" + to_string(fd);
	m_channel = new Channel(fd, FDEvent::ReadEvent, processRead, processWrite, destroy, this);
	m_evLoop->addTask(m_channel, ElemType::ADD);
}


TcpConnection::~TcpConnection()
{
	if (m_readBuf && m_readBuf->readableSize() == 0 &&
		m_writeBuf && m_writeBuf->readableSize() == 0) //判断这两个buffer是不是为空
	{
		delete m_readBuf;
		delete m_writeBuf;
		m_evLoop->freeChannel(m_channel);
		delete m_request;
		delete m_response;
	}
	Debug("连接断开，释放资源,connName:%s", m_name);
}
