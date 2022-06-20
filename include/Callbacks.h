#pragma once
#include <functional>
#include <memory>

class TcpConnection;
class Buffer;
class NetAddr;
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
typedef std::function<void(const TcpConnectionPtr &)> ConnectionCallback;
typedef std::function<void(const TcpConnectionPtr &, Buffer &)> MessageCallback;
typedef std::function<void(const TcpConnectionPtr &)> WriteCompleteCallback;
typedef std::function<void(const TcpConnectionPtr &)> CloseCallback;
typedef std::function<void(const TcpConnectionPtr &, size_t)>
    HighWaterMarkCallback;

typedef std::function<void(int sockfd, const NetAddr &peerAddr)>
    NewConnectionCallback;