#pragma once
#include <functional>
#include <iostream>
using namespace std;

//定义函数指针
//typedef int(*handleFunc)(void* arg);
//using handleFunc = int(*)(void*);//

//定义文件描述符的读写事件
enum class FDEvent //C++11强类型枚举
{
	TimeOut = 0x01,
	ReadEvent = 0x02,
	WriteEvent = 0x04
};

//可调用对象包装器打包的是什么：1.函数指针；2.可调用对象（可以像函数一样使用）
//最终得到了地址，但是没有调用
class Channel
{
public:
	using handleFunc = function<int(void*)>;//C++11 可调用对象包装器，包装一个函数指针或可调用对象
	//在面向对象中，如果使用函数指针则只能接收一个函数地址(普通函数或类的静态函数)，
	// 不能调用类的动态函数(在对象没有创建出来之前，没有这个函数)，类里面的非静态成员函数和成员变量都是属于一个对象的
	// 使用可调用对象包装器则可以调用任何函数。
	//初始化一个Channel
	Channel(int fd, FDEvent events, handleFunc readFunc, handleFunc writeFunc,
		handleFunc destroyFunc, void* arg);//这三个参数不是函数指针，而是可调用对象包装器类型
	
	//回调函数
	handleFunc readCallBack;
	handleFunc writeCallBack;
	handleFunc destroyCallBack;
	
	//修改fd的写事件（设置写事件被检测 or 不检测）
	void writeEventEnable(bool flag);
	//判断是否当前是否设置了检测写事件
	bool isWriteEventEnable();
	//取出私有成员
	inline int getEvent() {
		return m_events;
	}
	inline int getSocket() {
		return m_fd;
	}
	inline const void* getArg() {
		return m_arg;
	}
private:
	//文件描述符
	int m_fd;
	//事件
	int m_events;	
	//回调函数的参数
	void* m_arg;
};

