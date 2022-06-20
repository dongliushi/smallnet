#include "TcpConnection.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"
#include "Socket.h"
#include <memory>
#include <unistd.h>

TcpConnection::TcpConnection(EventLoop *loop, int sockfd,
                             const NetAddr &localAddr, const NetAddr &peerAddr)
    : loop_(loop), socket_(new Socket(sockfd)),
      state_(TcpConnectionState::Connecting),
      channel_(new Channel(loop, sockfd)), localAddr_(localAddr),
      peerAddr_(peerAddr) {
  channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this));
  channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
  channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
  channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
  socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection() {
  assert(state_ == TcpConnectionState::Disconnected);
}

void TcpConnection::connectEstablished() {
  loop_->assertInLoopThread();
  assert(state_ == TcpConnectionState::Connecting);
  state_ = TcpConnectionState::Connected;
  channel_->tie(shared_from_this());
  channel_->enableRead();
  connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed() {
  loop_->assertInLoopThread();
  if (state_ == TcpConnectionState::Connected) {
    setState(TcpConnectionState::Disconnected);
    channel_->disableAll();
    connectionCallback_(shared_from_this());
  }
  channel_->remove();
}

void TcpConnection::handleRead() {
  loop_->assertInLoopThread();
  int savedErrno = 0;
  ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
  if (n > 0) {
    messageCallback_(shared_from_this(), inputBuffer_);
  } else if (n == 0) {
    handleClose();
  } else {
    errno = savedErrno;
    LOG_DEBUG << "TcpConnection::handleRead";
    handleError();
  }
}

void TcpConnection::handleWrite() {
  loop_->assertInLoopThread();
  if (channel_->isWriting()) {
    ssize_t n = write(channel_->fd(), outputBuffer_.peek(),
                      outputBuffer_.readableBytes());
    if (n > 0) {
      outputBuffer_.retrieve(n);
      if (outputBuffer_.readableBytes() == 0) {
        channel_->disableWrite();
        if (writeCompleteCallback_) {
          loop_->queueInLoop(
              std::bind(writeCompleteCallback_, shared_from_this()));
        }
        if (state_ == TcpConnectionState::Disconnecting) {
          shutdownInLoop();
        }
      }
    } else {
      LOG_DEBUG << "TcpConnection::ERROR::handleWrite";
    }
  } else {
    LOG_DEBUG << "Connection fd = " << std::to_string(channel_->fd())
              << " is down, no more writing";
  }
}

void TcpConnection::handleClose() {
  loop_->assertInLoopThread();
  assert(state_ == TcpConnectionState::Connected ||
         state_ == TcpConnectionState::Disconnecting);
  setState(TcpConnectionState::Disconnected);
  channel_->disableAll();
  TcpConnectionPtr guardThis(shared_from_this());
  connectionCallback_(guardThis);
  closeCallback_(guardThis);
}

void TcpConnection::handleError() { LOG_DEBUG << "TcpConnection::handleError"; }

void TcpConnection::shutdownInLoop() {
  loop_->assertInLoopThread();
  if (!channel_->isWriting()) {
    socket_->shutdownWrite();
  }
}
