#pragma once
#include <string>
using namespace std;

class Buffer
{
public:
	//初始化
	Buffer(int size);
	//销毁内存
	~Buffer();
	//扩容函数
	void extendRoom(int size);
	//得到剩余可写的内存容量（没有被接收的数据占用的空间），还有多少空闲的数据可以写
	inline int writeableSize() {
		return m_capacity - m_readPos;
	}
	//得到剩余可读内存容量（在readbuf中没有被发送出去），还有多少数据在readbuf中没有读出
	inline int readableSize() {
		return m_writePos - m_readPos;
	}
	//写内存  直接写\接收套接字数据
	int appendString(const char* data, int size);
	int appendString(const char* data);
	int appendString(const string data);
	int socketRead(int fd);
	//根据\r\n取出一行，找到\r\n在数据块中的位置
	char* findCRLF();
	//发送数据
	int sendData(int socket);
	//得到读数据的起始位置
	inline char* data() {
		return m_data + m_readPos;
	}
	inline int readPosIncrease(int count) {
		m_readPos += count;
		return m_readPos;
	}
private:
	//指向内存的指针
	char* m_data;
	int m_capacity;
	int m_readPos = 0;
	int m_writePos = 0;//就地初始化 c++11
};
