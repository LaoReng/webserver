#pragma once
#include <string>
#include <chrono> // 时钟对应头文件

// 虚基类用于实现多态
class dbDispatcher
{
public:
    virtual ~dbDispatcher()
    {
        // printf("%s(%d)<%s>:析构函数被调用\n", __FILE__, __LINE__, __FUNCTION__);
    }
    // 连接数据库
    virtual bool connect(const std::string &, const std::string &, const std::string &, const std::string &, unsigned short) = 0;
    // 更新数据库：insert,update,delete
    virtual bool update(const std::string &) = 0;
    // 查询数据库
    virtual bool query(const std::string &) = 0;
    // 遍历查询得到的结果集
    virtual bool next() = 0;
    // 返回结果集中的列数
    virtual unsigned int getResNumFields()
    {
        return 0;
    }
    // 返回结果集中的行数
    virtual unsigned int getResNumRows()
    {
        return 0;
    }
    // 得到结果集中的字段值
    virtual std::string value(int) = 0;
    // 事务操作
    virtual bool transaction()
    {
        return false;
    }
    // 提交事务
    virtual bool commit()
    {
        return false;
    }
    // 事务回滚
    virtual bool rollback()
    {
        return false;
    }
    // 刷新起始的空闲时间点
    virtual void refreshAliveTime()
    {
        m_alivetime = std::chrono::steady_clock::now();
    }
    // 计算连接存活的总时长
    virtual long long getAliveTime()
    {
        std::chrono::nanoseconds res = std::chrono::steady_clock::now() - m_alivetime;
        std::chrono::milliseconds millsec = std::chrono::duration_cast<std::chrono::milliseconds>(res);
        return millsec.count();
    }

protected:
    std::chrono::steady_clock::time_point m_alivetime; // steady_clock：绝对时钟类 连接存活时间
};