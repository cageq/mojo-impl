//
// Created by titto on 2022/5/6.
//


#include "core/channel_posix.h"
#include "core/sock_ops.h"
#include "core/serializer.h"
#include "log/logging.h"

namespace tit {
namespace mojo {

void ChannelPosix::Start() {
  io_task_runner_->AddFdEvent(socket_, IOEvent::kReadable);
}

void ChannelPosix::Read() {

}

void ChannelPosix::Write(const Protocol::Ptr& protocol) {
  io_task_runner_->AddFdEvent(socket_, IOEvent::kWritable);
  writing_protocol_.push(protocol);
}

void ChannelPosix::ShutdownImpl() {
  io_task_runner_ = nullptr;
}

void ChannelPosix::OnFdReadable(int fd) {
  io_task_runner_->AddFdEvent(socket_, IOEvent::kWritable);
  Protocol::Ptr protocol = ReadHeader(Protocol::kBaseLen);
  LOG(DEBUG) << protocol->ToString();
  ports::UserMessage::Ptr user_msg =
      ReadBody(protocol, protocol->content_length());
  LOG(DEBUG) << user_msg->ToString();
}

void ChannelPosix::OnFdWriteable(int fd) {
  if (!writing_protocol_.empty()) {
    Protocol::Ptr protocol = writing_protocol_.front();
    writing_protocol_.pop();
    std::string data = Channel::Serialize(protocol);
    sock::Send(socket_, data.data(), data.size());
    LOG(DEBUG) << "send data: " << data
              << " size: " << data.size();
  }
  io_task_runner_->AddFdEvent(socket_, IOEvent::kReadable);
}

std::string ChannelPosix::ReadFixBufFromSocket(uint16 fix_len) const {
  char buf[fix_len+1];
  buf[fix_len] = '\0';
  sock::Recvn(socket_, buf, fix_len);
  return std::string(buf, fix_len);
}

Protocol::Ptr ChannelPosix::ReadHeader(int header_length) const {
  std::string buf = ReadFixBufFromSocket(header_length);
  return Channel::DeserializeProtocol(buf);
}

ports::UserMessage::Ptr ChannelPosix::ReadBody(const Protocol::Ptr& protocol,
                            int content_length) {
  std::string buf = ReadFixBufFromSocket(content_length);
  LOG(DEBUG) << "Recv body: " << buf;
  protocol->set_content(buf);
  return Channel::DeserializeMessage(buf);
}


}  // namespace mojo
}  // namespace tit