#include "Connector.h"
#include "Logger.h"
#include <cassert>
#include <sys/socket.h>
#include <unistd.h>

Connector::Connector(EventLoop *loop, const NetAddr &serverAddr)
    : loop_(loop), serverAddr_(serverAddr), state_(States::Disconnected),
      connected_(false), retryDelayMs_(InitRetryDelayMs) {}

Connector::~Connector() { assert(!channel_); }

void Connector::start() {
  connected_ = true;
  loop_->runInLoop(std::bind(&Connector::startInLoop, this));
}

void Connector::startInLoop() {
  loop_->assertInLoopThread();
  assert(state_ == States::Disconnected);
  if (connected_) {
    connect();
  } else {
    LOG_DEBUG << "do not connect";
  }
}

void Connector::stop() {
  connected_ = false;
  loop_->queueInLoop(std::bind(&Connector::stopInLoop, this));
}

void Connector::stopInLoop() {
  loop_->assertInLoopThread();
  if (state_ == States::Connecting) {
    setState(States::Disconnected);
    int sockfd = removeAndResetChannel();
    retry(sockfd);
  }
}

void Connector::connect() {
  int sockfd =
      socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
  assert(sockfd != -1);
  int ret = ::connect(sockfd, serverAddr_.getSockAddr(),
                      static_cast<socklen_t>(sizeof(sockaddr_in)));
  int savedErrno = (ret == 0) ? 0 : errno;
  switch (savedErrno) {
  case 0:
  case EINPROGRESS:
  case EINTR:
  case EISCONN:
    connecting(sockfd);
    break;
  case EAGAIN:
  case EADDRINUSE:
  case EADDRNOTAVAIL:
  case ECONNREFUSED:
  case ENETUNREACH:
    retry(sockfd);
    break;
  case EACCES:
  case EPERM:
  case EAFNOSUPPORT:
  case EALREADY:
  case EBADF:
  case EFAULT:
  case ENOTSOCK:
    close(sockfd);
    break;
  default:
    close(sockfd);
    break;
  }
}

void Connector::restart() {
  loop_->assertInLoopThread();
  setState(States::Disconnected);
  retryDelayMs_ = InitRetryDelayMs;
  connected_ = true;
  startInLoop();
}

void Connector::connecting(int sockfd) {
  setState(States::Connecting);
  assert(!channel_);
  channel_.reset(new Channel(loop_, sockfd));
  channel_->setWriteCallback(std::bind(&Connector::handleWrite, this));
  channel_->setErrorCallback(std::bind(&Connector::handleError, this));
  channel_->enableWrite();
}

int Connector::removeAndResetChannel() {
  channel_->disableAll();
  channel_->remove();
  int sockfd = channel_->fd();
  loop_->queueInLoop(
      std::bind(&Connector::resetChannel, this)); // FIXME: unsafe
  return sockfd;
}

void Connector::resetChannel() { channel_.reset(); }

int getSocketError(int sockfd) {
  int optval;
  socklen_t optlen = static_cast<socklen_t>(sizeof optval);
  if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
    return errno;
  } else {
    return optval;
  }
}

void Connector::handleWrite() {
  loop_->assertInLoopThread();
  if (state_ == States::Connecting) {
    int sockfd = removeAndResetChannel();
    int err = getSocketError(sockfd);
    if (err) {
      retry(sockfd);
    } else {
      setState(States::Connected);
      if (connected_) {
        newConnectionCallback_(sockfd);
      } else {
        close(sockfd);
      }
    }
  } else {
    assert(state_ == States::Disconnected);
  }
}

void Connector::handleError() {
  if (state_ == States::Connecting) {
    int sockfd = removeAndResetChannel();
    int err = getSocketError(sockfd);
    retry(sockfd);
  }
}

void Connector::retry(int sockfd) {
  close(sockfd);
  setState(States::Disconnected);
  if (connected_) {
    loop_->runAfter(retryDelayMs_,
                    std::bind(&Connector::startInLoop, shared_from_this()));
    retryDelayMs_ = std::min(retryDelayMs_ * 2, MaxRetryDelayMs);
  }
}
