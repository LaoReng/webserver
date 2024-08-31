#include "HttpResponse.h"
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>

HttpResponse::HttpResponse()
{
	m_statusCode = StatusCode::Unknown;
	//初始化数组
	m_headers.clear();
	m_filename = string();
	sendDataFunc = nullptr;
}

HttpResponse::~HttpResponse()
{
}

void HttpResponse::addHeader(const string key, const string value)
{
	if (key.empty() || value.empty())
	{
		return;
	}
	m_headers.insert(make_pair(key, value));
}

void HttpResponse::prepareMsg(Buffer* sendBuf, int socket)
{
	//状态行
	char tmp[1024] = { 0 };
	int code = static_cast<int>(m_statusCode);
	sprintf(tmp, "HTTP/1.1 %d %s\r\n", m_statusCode, m_info.at(code).data());
	sendBuf->appendString(tmp);
	//响应头
	for (auto it = m_headers.begin();it!=m_headers.end();++it)
	{
		sprintf(tmp, "%s: %s\r\n", it->first.data(), it->second.data());
		sendBuf->appendString(tmp);
	}
	//空行
	sendBuf->appendString("\r\n");
#ifndef MSG_SEND_AUTO
	sendBuf->sendData(socket);
#endif
	//回复的数据
	sendDataFunc(m_filename, sendBuf, socket);
}
