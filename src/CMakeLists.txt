find_package(Threads)

add_executable(test test.cc 
Acceptor.cc 
Buffer.cc 
Channel.cc
Epoller.cc
EventLoop.cc
EventLoopThread.cc
EventLoopThreadPool.cc
NetAddr.cc 
Socket.cc
TcpConnection.cc
TcpServer.cc
Logger.cc
)
target_link_libraries(test ${CMAKE_THREAD_LIBS_INIT})