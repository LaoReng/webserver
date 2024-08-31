#pragma once
#include "Buffer.h"
#include "HttpResponse.h"
#include <map>
using namespace std;

//当前的解析状态
enum class ProcessState:char
{
	ParseReqLine,//当前解析的是请求行
	ParseReqHeaders,//当前解析的是请求头
	ParseReqBody,//当前解析的是请求的数据块
	ParseReqDone//解析完了，可以回复数据了
};
//定义http请求结构体
class HttpRequest
{
public:
	//初始化
	HttpRequest();
	//重置与销毁
	~HttpRequest();
	void reset();
	//获取处理状态
	inline ProcessState getState() {
		return m_curState;
	}
	inline void setState(ProcessState state) {
		m_curState = state;
	}
	//添加请求头
	void addHeader(const string key, const string value);
	//获取请求头key对应的value
	string getHeader(const string key);
	//解析请求行
	bool parseRequestLine(Buffer* readBuf);
	//解析请求头
	bool parseRequestHeader(Buffer* readBuf);
	//解析http请求协议
	bool parseRequest(Buffer* readBuf, HttpResponse* response, Buffer* sendBuf, int socket);
	//处理http请求协议
	bool processRequest(HttpResponse* response);
	//解码
	string decodeMsg(string msg);
	const string getFileType(const string name);
	static void sendDir(const string dirName, Buffer* sendBuf, int socket);
	static void sendFile(const string fileName, Buffer* sendBuf, int socket);
	inline void setMethod(string method) {
		m_method = method;
	}
	inline void seturl(string url) {
		m_url = url;
	}
	inline void setVersion(string version) {
		m_version = version;
	}
private:
	char* splitRequestLine(const char* start, const char* end,
		const char* sub, function<void(string)> callback);
	int hexToDec(char c);
private:
	string m_method;
	string m_url;
	string m_version;
	map<string,string> m_reqHeaders;//请求头里面的键值对
	ProcessState m_curState;
};

