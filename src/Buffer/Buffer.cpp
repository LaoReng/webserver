// #define _GNU_SOURCE
#include "Buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <strings.h>

Buffer::Buffer(int size) : m_capacity(size)
{

	m_data = (char *)malloc(size); // 后续可能需要扩容，所以用malloc，而不用free
	bzero(m_data, size);
}

Buffer::~Buffer()
{
	if (m_data != nullptr)
	{
		free(m_data);
	}
}

void Buffer::extendRoom(int size)
{
	// 1.内存够用，不需要扩容
	if (writeableSize() >= size)
	{
		return;
	}
	// 2.内存需要合并才够用：不需要扩容
	// 剩余的可写内存+已读内存
	else if (m_readPos + writeableSize() >= size)
	{
		// 得到未读的内存大小
		int readable = readableSize();
		// 移动内存(把前面已读的空间移到后面和可写空间接起来，使其形成一个更大的可写空间)
		memcpy(m_data, m_data + m_readPos, readable);
		// 更新位置
		m_readPos = 0;
		m_writePos = readable;
	}
	// 3.内存不够用——扩容
	else
	{
		void *temp = realloc(m_data, m_capacity + size);
		if (temp == nullptr)
		{
			return; // 失败
		}
		memset((char *)temp + m_capacity, 0, size);
		// 更新数据
		m_data = static_cast<char *>(temp);
		m_capacity += size;
	}
}

int Buffer::appendString(const char *data, int size)
{
	if (data == nullptr || size <= 0)
	{
		return -1;
	}
	// 扩容
	extendRoom(size);
	// 数据拷贝
	memcpy(m_data + m_writePos, data, size);
	m_writePos += size;
	return 0;
}

int Buffer::appendString(const char *data)
{
	int size = strlen(data);
	int ret = appendString(data, size);
	return ret;
}

int Buffer::appendString(const string data)
{
	int ret = appendString(data.data());
	return ret;
}

int Buffer::socketRead(int fd)
{
	// 接收套接字数据 :read\recv\readv
	struct iovec vec[2];
	// 初始化数组元素
	int writeable = writeableSize();
	vec[0].iov_base = m_data + m_writePos;
	vec[0].iov_len = writeable;
	char *tmpbuf = (char *)malloc(40960);
	vec[1].iov_base = tmpbuf;
	vec[1].iov_len = 40960;
	int result = readv(fd, vec, 2); // 返回接收到了多少个字节
	if (result == -1)
	{
		return -1;
	}
	else if (result <= writeable)
	{
		// 全部写到了buffer的data中
		m_writePos += result;
	}
	else
	{
		// 数据不够，写到了第二个中,需要进行内存的扩展与拷贝
		m_writePos = m_capacity;
		appendString(tmpbuf, result - writeable);
	}
	free(tmpbuf);
	return result;
}

char *Buffer::findCRLF()
{
	// strstr：大字符串匹配子字符串(遇到\0结束)
	// memmem:大数据块匹配子数据块（需要指定数据块大小）
	char *ptr = (char *)memmem(m_data + m_readPos, readableSize(), "\r\n", 2);
	return ptr;
}

int Buffer::sendData(int socket)
{
	// 判断有无数据
	int readable = readableSize(); // 不要调用WriteableSize，ReadableSize是用来判断
	// readbuf中还有多少没有处理的数据，readbuf中存放的是前面解析并准备好的要回送的http协议的内容
	// 即是用来发送的数据。
	if (readable > 0)
	{
		int count = send(socket, m_data + m_readPos, readable, MSG_NOSIGNAL);
		if (count)
		{
			m_readPos += count;
			usleep(1);
		}
		return count;
	}
	return 0;
}
