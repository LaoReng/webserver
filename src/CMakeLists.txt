CMAKE_MINIMUM_REQUIRED(VERSION 3.22.1)
PROJECT(subCMake)

FIND_PACKAGE(Threads REQUIRED)

# 指定头文件所在位置
INCLUDE_DIRECTORIES(
    Buffer
    IO-Multiplexing
    IO-Multiplexing/Epoll
    IO-Multiplexing/Poll
    IO-Multiplexing/Select
    DB-ConnectionPool/ConnectionPool
    DB-ConnectionPool/DBDispatcher
    DB-ConnectionPool/DBDispatcher/MySQL
    jsoncpp
    EventLoop
    Http
    Logger
    TCP
    Thread
)

# 搜索源文件和指定头文件所在的文件夹
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR} SRC)

# 添加宏定义在程序里面用
ADD_DEFINITIONS(-DDB_CONFIG_PATH="${CMAKE_CURRENT_SOURCE_DIR}/config/dbconf.json")

# 添加子cmake文件
ADD_SUBDIRECTORY(Buffer) # 包含jsoncpp子目录下面的CMake文件
ADD_SUBDIRECTORY(IO-Multiplexing) # 包含logger子目录下面的CMake文件
ADD_SUBDIRECTORY(IO-Multiplexing/Epoll)
ADD_SUBDIRECTORY(IO-Multiplexing/Poll)
ADD_SUBDIRECTORY(IO-Multiplexing/Select)
ADD_SUBDIRECTORY(EventLoop)
ADD_SUBDIRECTORY(Http)
ADD_SUBDIRECTORY(Logger)
ADD_SUBDIRECTORY(TCP)
ADD_SUBDIRECTORY(Thread)
ADD_SUBDIRECTORY(DB-ConnectionPool)
ADD_SUBDIRECTORY(jsoncpp)

# 生成可执行程序
ADD_EXECUTABLE(${EXE_NAME} ${SRC})

# 连接动态库
TARGET_LINK_LIBRARIES(
    ${EXE_NAME}
    Threads::Threads
    mysqlclient
)
