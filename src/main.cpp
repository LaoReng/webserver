#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <iostream>
#include <thread>
#include <assert.h>
#include "TcpServer.h"
#include "MysqlDispatcher.h"
#include "ConnectionPool.h"

int query()
{

    dbDispatcher *conn = new MysqlDispatcher;

    // MysqlDispatcher conn;
    conn->connect("root", "123", "CrossTime", "192.168.10.132", 3306);
    std::string sql = "insert into vip VALUES(3,0,0,0,\'2024-04-22 19:35:16\',0,0,0);";
    bool ret = conn->update(sql);
    std::cout << "ret =  " << ret << std::endl;

    sql = "select * from vip;";
    conn->query(sql);
    printf("结果集共有%d条（%d列）\n", conn->getResNumRows(), conn->getResNumFields());
    while (conn->next())
    {
        std::cout << conn->value(0) << ","
                  << conn->value(1) << ","
                  << conn->value(2) << ","
                  << conn->value(3) << ","
                  << conn->value(4) << ","
                  << conn->value(5) << ","
                  << conn->value(6) << ","
                  << conn->value(7) << std::endl;
    }
    delete conn;
    return 0;
}

// 1.单线程：使用/不使用连接池
// 2.多线程：使用/不使用连接池

void op1(int begin, int end)
{
    for (int i = begin; i < end; ++i)
    {
        MysqlDispatcher conn;
        conn.connect("root", "123", "CrossTime", "192.168.10.132");
        char sql[512] = "";
        sprintf(sql, "insert into manager VALUES(%d,'admin','15046543174','admin',\'2024-04-22 19:35:16\');", i + 3);
        bool ret = conn.update(sql);
        assert(ret == 1);
        // std::cout << "ret =  " << ret << std::endl;
    }
}

void op2(ConnectionPool *pool, int begin, int end)
{
    for (int i = begin; i < end; ++i)
    {
        std::shared_ptr<dbDispatcher> conn = pool->getConnection();
        char sql[512] = "";
        sprintf(sql, "insert into manager VALUES(%d,'admin','15046543174','admin',\'2024-04-22 19:35:16\');", i + 3);
        bool ret = conn->update(sql);
        // std::cout << "ret =  " << ret << std::endl;
        assert(ret == 1); // 判断条件为false的时候进行断言
    }
}

void test1()
{
#if 0
    // 非连接池（单线程），用时：24296211352纳秒/24296毫秒
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    op1(1, 5000);
    auto length = std::chrono::steady_clock::now() - begin;
    std::cout << "非连接池（单线程），用时：" << length.count() << "纳秒/" << length.count() / 1000000 << "毫秒" << std::endl;
#else
    // 连接池（单线程），用时：4103881348纳秒/4103毫秒
    ConnectionPool *conn = ConnectionPool::getConnectPool();
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    op2(conn, 1, 5000);
    auto length = std::chrono::steady_clock::now() - begin;
    std::cout << "连接池（单线程），用时：" << length.count() << "纳秒/" << length.count() / 1000000 << "毫秒" << std::endl;
#endif
}

void test2()
{
#if 0
    // 非连接池（多线程），用时：7646395533纳秒/7646毫秒
    MysqlDispatcher conn;
    conn.connect("root", "123", "CrossTime", "192.168.10.132");
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    std::thread t1(op1, 0, 1000);
    std::thread t2(op1, 1001, 2000);
    std::thread t3(op1, 2001, 3000);
    std::thread t4(op1, 3001, 4000);
    std::thread t5(op1, 4001, 5000);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    auto length = std::chrono::steady_clock::now() - begin;
    std::cout << "非连接池（多线程），用时：" << length.count() << "纳秒/" << length.count() / 1000000 << "毫秒" << std::endl;
#else
    // 连接池（多线程），用时：1228784889纳秒/1228毫秒
    ConnectionPool *conn = ConnectionPool::getConnectPool();
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    std::thread t1(op2, conn, 0, 1000);
    std::thread t2(op2, conn, 1001, 2000);
    std::thread t3(op2, conn, 2001, 3000);
    std::thread t4(op2, conn, 3001, 4000);
    std::thread t5(op2, conn, 4001, 5000);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();
    auto length = std::chrono::steady_clock::now() - begin;
    std::cout << "连接池（多线程），用时：" << length.count() << "纳秒/" << length.count() / 1000000 << "毫秒" << std::endl;
#endif
}

int main()
{
    // query();
    test1();
    // printf("%s(%d):主线程结束运行\n", __FILE__, __LINE__);

#if 0
    if (argc < 3)
    {
        printf("./a.out port path\n");
        return -1;
    }
    unsigned short port = atoi(argv[1]);
    // 切换服务器的工作路径
    chdir(argv[2]);
#else
    unsigned short port = 10000;
    chdir("/");
#endif
    // 启动服务器
    TcpServer *server = new TcpServer(port, 4);
    server->run();
    return 0;
}
