#pragma once
#include "EventLoop.h"
#include "Buffer.h"
#include "Channel.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

//#define MSG_SEND_AUTO 
//用来切换两种发送数据的方式:1.一种是全部接收完，组织好后，添加写监听，
//然后检测到写事件后一次性发送；2.一种是不添加写监听，在读事件触发组织http数据块时
//就组织一部分发一部分


class TcpConnection
{
public:
	TcpConnection(int fd, EventLoop* evLoop);
	~TcpConnection();
	// TCP 读数据
	static int processRead(void* arg);
	// TCP 写数据
	static int processWrite(void* arg);
	// TCP 销毁
	static int destroy(void* arg);
private:
	EventLoop* m_evLoop;
	Channel* m_channel;
	Buffer* m_readBuf;
	Buffer* m_writeBuf;
	string m_name;
	//http协议
	HttpRequest* m_request;
	HttpResponse* m_response;
};