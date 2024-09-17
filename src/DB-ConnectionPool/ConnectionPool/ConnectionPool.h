#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "MysqlDispatcher.h"


/// @brief 数据库连接池类（单例模式）
class ConnectionPool
{
public:
    /// @brief 返回数据库连接池的全局唯一对象指针
    /// @return
    static ConnectionPool *getConnectPool();
    ConnectionPool(const ConnectionPool &con) = delete;
    ConnectionPool &operator=(const ConnectionPool &con) = delete;
    /// @brief 获取连接池中的一个连接
    /// @return 返回一个可用的数据库连接
    // std::shared_ptr<MysqlDispatcher> getConnection();
    std::shared_ptr<dbDispatcher> getConnection();
    ~ConnectionPool();
private:
    ConnectionPool();
    /// @brief 解析json格式文件
    bool parseJsonFile();
    // 线程函数
    /// @brief 生产连接池里面的连接
    void produceConnection();
    /// @brief 检测有没有需要销毁的连接
    void recycleConnection();
    /// @brief 创建数据库连接
    void addConnection();

private:
    std::string m_ip;
    std::string m_user;
    std::string m_passwd;
    std::string m_dbName;
    unsigned short m_port;
    int m_minSize;     // 数据库连接对象下限，默认创建的最小连接数
    int m_maxSize;     // 数据库连接对象上限
    int m_timeout;     // 超时时长，用于无可用连接时服务器的等待时长
    int m_maxIdleTime; // 数据库连接空闲时长
    // std::queue<MysqlDispatcher *> m_connectionQ;
    std::queue<dbDispatcher *> m_connectionQ;
    std::mutex m_mutexQ;            // 互斥锁
    std::condition_variable m_cond; // 条件变量
    // 在Linux下使用detach将线程分离，但是这样并不会时主线程能够正常结束
    std::atomic<bool> isRun;  // 程序是否在运行（原子变量）
};