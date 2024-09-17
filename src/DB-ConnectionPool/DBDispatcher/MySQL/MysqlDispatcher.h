#pragma once
#include <iostream>
#include <string>
#include <mysql/mysql.h>
#include <chrono> // 时钟对应头文件
#include "dbDispatcher.h"

// mysql数据库API类
class MysqlDispatcher : public dbDispatcher
{
public:
    // 初始化数据库连接
    MysqlDispatcher();
    // 释放数据库连接
    virtual ~MysqlDispatcher();
    // 连接数据库
    virtual bool connect(const std::string &user, const std::string &passwd, const std::string &dbName, const std::string &ip, unsigned short port = 3306) override;
    // 更新数据库：insert,update,delete
    virtual bool update(const std::string &sql) override;
    // 查询数据库
    virtual bool query(const std::string &sql) override;
    // 遍历查询得到的结果集
    virtual bool next() override;
    // 返回结果集中的列数
    virtual unsigned int getResNumFields() override;
    // 返回结果集中的行数
    virtual unsigned int getResNumRows() override;
    // 得到结果集中的字段值
    virtual std::string value(int index) override;
    // 事务操作
    virtual bool transaction() override;
    // 提交事务
    virtual bool commit() override;
    // 事务回滚
    virtual bool rollback() override;

private:
    void freeResult();

private:
    MYSQL *m_conn = nullptr;
    MYSQL_RES *m_result = nullptr;
    MYSQL_ROW m_row = nullptr;
};