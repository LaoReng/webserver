#pragma once
#include "Buffer.h"
#include <map>
#include <iostream>
#include <functional>
using namespace std;

//定义状态码
enum class StatusCode
{
	Unknown,
	OK = 200,
	MovedPermanently = 301,
	MovedTemporarily = 302,
	BadRequest = 400,
	NotFound = 404
};

class HttpResponse
{
public:
	function<void(const string, Buffer*, int)> sendDataFunc;
	//初始化
	HttpResponse();
	//销毁
	~HttpResponse();
	//添加响应头
	void addHeader(const string key, const string value);
	//组织http响应数据
	void prepareMsg(Buffer* sendBuf, int socket);
	inline void setFileName(string name) {
		m_filename = name;
	}
	inline void setStatusCode(StatusCode code) {
		m_statusCode = code;
	}
private:
	//状态行：状态码，状态描述，http协议版本
	StatusCode m_statusCode;
	string m_filename;
	//响应头-键值对
	map<string, string> m_headers;
	//状态码和状态描述
	const map<int, string> m_info = {
		{200, "OK"},
		{301, "MovedPermanently"},
		{302, "MovedTemporarily"},
		{400, "BadRequest"},
		{404, "NotFound"},
	};
};


