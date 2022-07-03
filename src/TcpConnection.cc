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

void TcpConnection::shutdown() {
  if (state_ == TcpConnectionState::Connected) {
    setState(TcpConnectionState::Disconnecting);
    loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
  }
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
    LOG_DEBUG << "Connection fd = " << channel_->fd()
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

void TcpConnection::send(const std::string &data) {
  if (state_ == TcpConnectionState::Connected) {
    if (loop_->isInLoopThread()) {
      sendInLoop(data);
    }
  }
}

void TcpConnection::sendInLoop(const std::string &data) {
  loop_->assertInLoopThread();
  size_t len = data.size();
  ssize_t nwrote = 0;
  size_t remaining = len;
  if (state_ == TcpConnectionState::Disconnected) {
    LOG_DEBUG << "disconnected, give up writing";
    return;
  }
  if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
    nwrote = write(channel_->fd(), data.c_str(), len);
    if (nwrote >= 0) {
      remaining = len - nwrote;
      if (remaining == 0 && writeCompleteCallback_) {
        loop_->queueInLoop(
            std::bind(writeCompleteCallback_, shared_from_this()));
      }
    } else {
      nwrote = 0;
      if (errno != EWOULDBLOCK) {
        LOG_DEBUG << "TcpConnection::sendInLoop";
      }
    }
  }
  assert(remaining <= len);
  if (remaining > 0) {
    outputBuffer_.append(data.c_str() + nwrote, remaining);
    if (!channel_->isWriting()) {
      channel_->enableWrite();
    }
  }
}

void TcpConnection::forceClose() {
  if (state_ == TcpConnectionState::Connected ||
      state_ == TcpConnectionState::Disconnecting) {
    setState(TcpConnectionState::Disconnecting);
    loop_->queueInLoop(
        std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
  }
}

void TcpConnection::forceCloseInLoop() {
  loop_->assertInLoopThread();
  if (state_ == TcpConnectionState::Connected ||
      state_ == TcpConnectionState::Disconnecting) {
    handleClose();
  }
}
