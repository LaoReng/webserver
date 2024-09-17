#include "MysqlDispatcher.h"

MysqlDispatcher::MysqlDispatcher()
{
    m_conn = mysql_init(nullptr);
    mysql_set_character_set(m_conn, "utf8");
}

MysqlDispatcher::~MysqlDispatcher()
{
    // printf("%s(%d)<%s>:析构函数被调用\n", __FILE__, __LINE__, __FUNCTION__);

    freeResult();
    if (m_conn != nullptr)
    {
        mysql_close(m_conn);
        m_conn = nullptr;
    }
}

bool MysqlDispatcher::connect(const std::string &user, const std::string &passwd, const std::string &dbName, const std::string &ip, unsigned short port)
{
    MYSQL *ptr = mysql_real_connect(m_conn, ip.c_str(), user.c_str(), passwd.c_str(), dbName.c_str(), port, nullptr, 0);
    refreshAliveTime();
    return ptr != nullptr;
}

bool MysqlDispatcher::update(const std::string &sql)
{
    if (mysql_query(m_conn, sql.c_str()))
    {
        return false;
    }
    return true;
}

bool MysqlDispatcher::query(const std::string &sql)
{
    freeResult();
    if (mysql_query(m_conn, sql.c_str()))
    {
        return false;
    }
    m_result = mysql_store_result(m_conn);
    return true;
}

bool MysqlDispatcher::next()
{
    if (m_result != nullptr)
    {
        // printf("error:执行到这里\n");
        if (m_row = mysql_fetch_row(m_result))
            return true;
    }
    return false;
}

unsigned int MysqlDispatcher::getResNumFields()
{
    if (m_result == nullptr)
        return 0;
    return mysql_num_fields(m_result);
}

unsigned int MysqlDispatcher::getResNumRows()
{
    if (m_result == nullptr)
        return 0;
    return mysql_num_rows(m_result);
}

std::string MysqlDispatcher::value(int index)
{
    int rowCount = mysql_num_fields(m_result);
    if (index >= rowCount || index < 0)
    {
        return std::string();
    }
    char *val = m_row[index];
    unsigned long length = mysql_fetch_lengths(m_result)[index];
    return std::string(val, length);
}

bool MysqlDispatcher::transaction()
{
    return mysql_autocommit(m_conn, false);
}

bool MysqlDispatcher::commit()
{
    return mysql_commit(m_conn);
}

bool MysqlDispatcher::rollback()
{
    return mysql_rollback(m_conn);
}

void MysqlDispatcher::freeResult()
{
    if (m_result)
    {
        mysql_free_result(m_result);
        m_result = nullptr;
        m_row = nullptr;
    }
}
