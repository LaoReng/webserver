CMAKE_MINIMUM_REQUIRED(VERSION 3.22.1)
PROJECT(subCMake)

FIND_PACKAGE(Threads REQUIRED)

# 指定头文件所在位置
INCLUDE_DIRECTORIES(
    Buffer
    Dispatcher
    Epoll
    EventLoop
    Http
    Logger
    Poll
    Select
    TCP
    Thread
)
# 搜索源文件和指定头文件所在的文件夹
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR} SRC)

# 添加子cmake文件
ADD_SUBDIRECTORY(Buffer)   # 包含jsoncpp子目录下面的CMake文件
ADD_SUBDIRECTORY(Dispatcher)    # 包含logger子目录下面的CMake文件
ADD_SUBDIRECTORY(Epoll)
ADD_SUBDIRECTORY(EventLoop)
ADD_SUBDIRECTORY(Http)
ADD_SUBDIRECTORY(Logger)
ADD_SUBDIRECTORY(Poll)
ADD_SUBDIRECTORY(Select)
ADD_SUBDIRECTORY(TCP)
ADD_SUBDIRECTORY(Thread)



# 生成可执行程序
ADD_EXECUTABLE(${EXE_NAME} ${SRC})

# 连接动态库
TARGET_LINK_LIBRARIES(${EXE_NAME} Threads::Threads)