#define _GNU_SOURCE
#include "HttpRequest.h"
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include "TcpConnection.h"
#include <assert.h>
#include <ctype.h>
#include "Log.h"

HttpRequest::HttpRequest()
{
	reset();
}

HttpRequest::~HttpRequest()
{
}

void HttpRequest::reset()
{
	m_curState = ProcessState::ParseReqLine;
	m_method = m_url = m_version = string();
	m_reqHeaders.clear();
}

void HttpRequest::addHeader(const string key, const string value)
{
	if (key.empty() || value.empty())
	{
		return;
	}
	m_reqHeaders.insert(make_pair(key, value));
}

string HttpRequest::getHeader(const string key)
{
	auto item = m_reqHeaders.find(key);
	if (item == m_reqHeaders.end())
	{
		return string();
	}
	return item->second;
}

bool HttpRequest::parseRequestLine(Buffer* readBuf)
{
	//读出请求行 保存字符串结束地址
	char* end = readBuf->findCRLF();
	//保存字符串起始地址
	char* start = readBuf->data();
	//请求行总长度
	int lineSize = end - start;
	if (lineSize > 0)
	{
		auto methodFunc = bind(&HttpRequest::setMethod, this, placeholders::_1);//可调用对象绑定
		start = splitRequestLine(start, end, " ", methodFunc);
		auto urlFunc = bind(&HttpRequest::seturl, this, placeholders::_1);//可调用对象绑定
		start = splitRequestLine(start, end, " ", urlFunc);
		auto versionFunc = bind(&HttpRequest::setVersion, this, placeholders::_1);//可调用对象绑定
		start = splitRequestLine(start, end, nullptr, versionFunc);
		//为解析请求头做准备
		readBuf->readPosIncrease(lineSize + 2);
		//修改状态
		setState(ProcessState::ParseReqHeaders);
		return true;
	}
	return false;
}

//该函数处理请求头中的一行，获取其键值对
bool HttpRequest::parseRequestHeader(Buffer* readBuf)
{
	char* end = readBuf->findCRLF();
	if (end != nullptr)
	{
		char* start = readBuf->data();
		int lineSize = end - start;
		//基于:搜索字符串
		char* middle = static_cast<char*>(memmem(start, lineSize, ": ", 2));

		if (middle != nullptr)
		{
			int keyLen = middle - start;
			int valueLen = end - middle - 2;
			if (keyLen > 0 && valueLen > 0)
			{
				string key(start, keyLen);
				string value(middle + 2, valueLen);
				addHeader(key, value);
			}
			//移动读数据的位置，到下一行，读下一行的键值对
			readBuf->readPosIncrease(lineSize + 2);
		}
		else
		{
			//请求头被解析完了，跳过空行
			readBuf->readPosIncrease(2);
			//修改解析状态
			//忽略post请求，按照get请求处理，get请求没有请求体，下一状态就是done
			setState(ProcessState::ParseReqDone);
		}
		return true;
	}
	return false;
}

bool HttpRequest::parseRequest(Buffer* readBuf, HttpResponse* response, Buffer* sendBuf, int socket)
{
	bool flag = true;
	while (m_curState != ProcessState::ParseReqDone)
	{
		switch (m_curState)
		{
		case ProcessState::ParseReqLine:
			flag = parseRequestLine(readBuf);
			break;
		case ProcessState::ParseReqHeaders:
			flag = parseRequestHeader(readBuf);
			break;
		case ProcessState::ParseReqBody:
			break;
		default:
			break;
		}
		if (!flag)
		{
			return flag;
		}
		//判断是否解析完毕，如果解析完了，需要准备回复数据
		if (m_curState == ProcessState::ParseReqDone)
		{
			Debug("解析完毕");
			//1.处理http请求，组织好准备回复的数据，把数据存储到response中
			processRequest(response);
			Debug("数据组织成功");
			//2.从response结构体中取出数据，通过http组织成一个http数据块(在sendbuf中)，准备发送
			response->prepareMsg(sendBuf, socket);
		}
	}
	setState(ProcessState::ParseReqLine);//状态还原，保证还能继续处理第二条及以后的请求
	return flag;
}

//处理客户端基于get的http请求
bool HttpRequest::processRequest(HttpResponse* response)
{
	if (strcasecmp(m_method.data(), "get") != 0)
	{
		return -1;
	}
	m_url = decodeMsg(m_url);//转码，防止中文乱码
	// 处理客户端请求的静态资源(目录或者文件)
	const char* file = nullptr;
	if (strcmp(m_url.data(), "/") == 0)//访问的服务器提供的资源目录的根目录
	{
		file = "./";
	}
	else
	{
		file = m_url.data() + 1;//是相对路径
	}
	// 获取文件属性
	struct stat st;
	int ret = stat(file, &st);
	if (ret == -1)
	{
		// 文件不存在 -- 回复404
		response->setFileName("404.html");
		response->setStatusCode(StatusCode::NotFound);
		//响应头
		response->addHeader("Content-type", getFileType(".html"));
		response->sendDataFunc = sendFile;
		return 0;
	}

	response->setFileName(file);
	response->setStatusCode(StatusCode::OK);
	// 判断文件类型
	//把要发送的内容先进行存储，存储到response结构体中
	//再调用response的httpResponsePrepareMsg函数进行发送
	if (S_ISDIR(st.st_mode))
	{
		//响应头
		response->addHeader("Content-type", getFileType(".html"));
		response->sendDataFunc = sendDir;
	}
	else
	{
		//响应头
		char tmp[12] = { 0 };
		response->addHeader("Content-type", getFileType(file));
		response->addHeader("Content-length", to_string(st.st_size));
		response->sendDataFunc = sendFile;
	}
	return false;
}

// 解码
// to 存储解码之后的数据, 传出参数, from被解码的数据, 传入参数
string HttpRequest::decodeMsg(string msg)
{
	string str = string();
	const char* from = msg.data();
	for (; *from != '\0'; ++from)
	{
		// isxdigit -> 判断字符是不是16进制格式, 取值在 0-f
		// Linux%E5%86%85%E6%A0%B8.jpg
		if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
		{
			// 将16进制的数 -> 十进制 将这个数值赋值给了字符 int -> char
			// B2 == 178
			// 将3个字符, 变成了一个字符, 这个字符就是原始数据
			str.append(1, hexToDec(from[1]) * 16 + hexToDec(from[2]));
			// 跳过 from[1] 和 from[2] 因此在当前循环中已经处理过了
			from += 2;
		}
		else
		{
			// 字符拷贝, 赋值
			str.append(1, *from);
		}
	}
	str.append(1, '\0');
	return str;
}

const string HttpRequest::getFileType(const string name)
{
	// a.jpg a.mp4 a.html
	// 自右向左查找‘.’字符, 如不存在返回NULL
	const char* dot = strrchr(name.data(), '.');
	if (dot == NULL)
		return "text/plain; charset=utf-8";	// 纯文本
	if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
		return "text/html; charset=utf-8";
	if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
		return "image/jpeg";
	if (strcmp(dot, ".gif") == 0)
		return "image/gif";
	if (strcmp(dot, ".png") == 0)
		return "image/png";
	if (strcmp(dot, ".css") == 0)
		return "text/css";
	if (strcmp(dot, ".au") == 0)
		return "audio/basic";
	if (strcmp(dot, ".wav") == 0)
		return "audio/wav";
	if (strcmp(dot, ".avi") == 0)
		return "video/x-msvideo";
	if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
		return "video/quicktime";
	if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0)
		return "video/mpeg";
	if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
		return "model/vrml";
	if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
		return "audio/midi";
	if (strcmp(dot, ".mp3") == 0)
		return "audio/mpeg";
	if (strcmp(dot, ".ogg") == 0)
		return "application/ogg";
	if (strcmp(dot, ".pac") == 0)
		return "application/x-ns-proxy-autoconfig";

	return "text/plain; charset=utf-8";
}

/*
<html>
	<head>
		<title>test</title>
	</head>
	<body>
		<table>
			<tr>
				<td></td>
				<td></td>
			</tr>
			<tr>
				<td></td>
				<td></td>
			</tr>
		</table>
	</body>
</html>
*/

void HttpRequest::sendDir(const string dirName, Buffer* sendBuf, int socket)
{
	//拼了一个html的网页
	char buf[4096] = { 0 };
	sprintf(buf, "<html><head><title>%s</title></head><body><table>", dirName.data());
	struct dirent** namelist;
	int num = scandir(dirName.data(), &namelist, NULL, alphasort);
	for (int i = 0; i < num; ++i)
	{
		// 取出文件名 namelist 指向的是一个指针数组 struct dirent* tmp[]
		char* name = namelist[i]->d_name;
		struct stat st;
		char subPath[1024] = { 0 };
		sprintf(subPath, "%s/%s", dirName.data(), name);
		stat(subPath, &st);
		if (S_ISDIR(st.st_mode))
		{
			// a标签 <a href="">name</a>
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s/\">%s</a></td><td>%ld</td></tr>",
				name, name, st.st_size);
		}
		else
		{
			sprintf(buf + strlen(buf),
				"<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
				name, name, st.st_size);
		}
		//send(cfd, buf, strlen(buf), 0);
		sendBuf->appendString(buf);
#ifndef MSG_SEND_AUTO
		sendBuf->sendData(socket);
#endif
		memset(buf, 0, sizeof(buf));
		free(namelist[i]);
	}
	sprintf(buf, "</table></body></html>");
	//send(cfd, buf, strlen(buf), 0);
	sendBuf->appendString(buf);
#ifndef MSG_SEND_AUTO
	sendBuf->sendData(socket);
#endif
	free(namelist);
}

void HttpRequest::sendFile(const string fileName, Buffer* sendBuf, int socket)
{
	// 1. 打开文件
	int fd = open(fileName.data(), O_RDONLY);
	if(fd <= 0){
		printf("文件打开失败 <%s>\n", fileName.c_str());
	}
	assert(fd > 0);
#if 1
	while (1)
	{
		char buf[1024];
		int len = read(fd, buf, sizeof buf);
		if (len > 0)
		{
			//send(cfd, buf, len, 0);
			sendBuf->appendString(buf);
#ifndef MSG_SEND_AUTO
			sendBuf->sendData(socket);
#endif
		}
		else if (len == 0)
		{
			break;
		}
		else
		{
			close(fd);
			perror("read");
		}
	}
#else
	off_t offset = 0;
	int size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	while (offset < size)
	{
		int ret = sendfile(cfd, fd, &offset, size - offset);
		printf("ret value: %d\n", ret);
		if (ret == -1 && errno == EAGAIN)
		{
			printf("没数据...\n");
		}
	}
#endif
	close(fd);
}

char* HttpRequest::splitRequestLine(const char* start, const char* end, const char* sub, function<void(string)> callback)
{
	char* space = const_cast<char*>(end);
	if (sub != nullptr)
	{
		space = static_cast<char*>(memmem(start, end - start, sub, strlen(sub)));
		assert(space != nullptr);
	}
	int length = space - start;
	callback(string(start, length));
	return space + 1;
}

int HttpRequest::hexToDec(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;

	return 0;
}
