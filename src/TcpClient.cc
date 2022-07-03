#include "TcpClient.h"
#include "Connector.h"
#include "Logger.h"
#include <cstring>
#include <sys/socket.h>

using namespace std::placeholders;

struct sockaddr_in getLocalAddr(int sockfd) {
  struct sockaddr_in localaddr;
  memset(&localaddr, 0, sizeof localaddr);
  socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
  if (getsockname(sockfd, reinterpret_cast<sockaddr *>(&localaddr), &addrlen) <
      0) {
    LOG_DEBUG << "sockets::getLocalAddr";
  }
  return localaddr;
}

struct sockaddr_in getPeerAddr(int sockfd) {
  struct sockaddr_in peeraddr;
  memset(&peeraddr, 0, sizeof peeraddr);
  socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
  if (getpeername(sockfd, reinterpret_cast<sockaddr *>(&peeraddr), &addrlen) <
      0) {
    LOG_DEBUG << "sockets::getPeerAddr";
  }
  return peeraddr;
}
namespace detail {
void removeConnection(EventLoop *loop, const TcpConnectionPtr &conn) {
  loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}

void removeConnector(const TcpClient::ConnectorPtr &connector) {
  // connector->
}
} // namespace detail
TcpClient::TcpClient(EventLoop *loop, const NetAddr &serverAddr)
    : loop_(loop), connector_(new Connector(loop, serverAddr)),
      connectionCallback_(defaultConnectionCallback),
      messageCallback_(defaultMessageCallback), retry_(false), connect_(true) {
  connector_->setNewConnectionCallback(
      std::bind(&TcpClient::newConnection, this, _1));
}

TcpClient::~TcpClient() {
  TcpConnectionPtr conn;
  bool unique = false;
  {
    std::unique_lock<std::mutex> lock(mutex_);
    unique = connection_.unique();
    conn = connection_;
  }
  if (conn) {
    assert(loop_ == conn->getLoop());
    CloseCallback cb = std::bind(&detail::removeConnection, loop_, _1);
    loop_->runInLoop(std::bind(&TcpConnection::setCloseCallback, conn, cb));
    if (unique) {
      conn->forceClose();
    }
  } else {
    connector_->stop();
    loop_->runAfter(Timer::milliseconds(1),
                    std::bind(&detail::removeConnector, connector_));
  }
}

void TcpClient::connect() {
  connect_ = true;
  connector_->start();
}

void TcpClient::disconnect() {
  connect_ = false;
  {
    std::unique_lock<std::mutex> lock(mutex_);
    if (connection_) {
      connection_->shutdown();
    }
  }
}

void TcpClient::stop() {
  connect_ = false;
  connector_->stop();
}

void TcpClient::newConnection(int sockfd) {
  loop_->assertInLoopThread();
  NetAddr peerAddr(getPeerAddr(sockfd));
  NetAddr localAddr(getLocalAddr(sockfd));
  TcpConnectionPtr conn(new TcpConnection(loop_, sockfd, localAddr, peerAddr));
  conn->setConnectionCallback(connectionCallback_);
  conn->setMessageCallback(messageCallback_);
  conn->setWriteCompleteCallback(writeCompleteCallback_);
  conn->setCloseCallback(
      std::bind(&TcpClient::removeConnection, this, _1)); // FIXME: unsafe
  {
    std::unique_lock<std::mutex> lock(mutex_);
    connection_ = conn;
  }
  conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr &conn) {
  loop_->assertInLoopThread();
  assert(loop_ == conn->getLoop());
  {
    std::unique_lock<std::mutex> lock(mutex_);
    assert(connection_ == conn);
    connection_.reset();
  }
  loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
  if (retry_ && connect_) {
    connector_->restart();
  }
}