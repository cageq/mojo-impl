#pragma once

#include <unistd.h>
#include <stdlib.h>

#include "log/logging.h"
#include "base/process.h"
#include "core/libuv_io_task_runner_adapter.h"
#include "core/channel_posix.h"
#include "core/sock_ops.h"
#include "core/protocol.h"
#include "core/ports/user_message.h"

#include <iostream>
#include <vector>


using namespace tit;

void AddParentPrefix(
    log::LogStream& stream,
    const log::LogMessageInfo& l,
    void* data) {
  stream << "[parent] " << l.level_
         << "20220406-18:30:30"
         << ' ' << PlatformThread::CurrentId()
         << ' ' << l.fullname_ << ':' << l.line_ << ' ';
}

void AddChildPrefix(
    log::LogStream& stream,
    const log::LogMessageInfo& l,
    void* data) {
  stream << "[Child ] " << l.level_
         << "20220406-18:30:30"
         << ' ' << PlatformThread::CurrentId()
         << ' ' << l.fullname_ << ':' << l.line_ << ' ';
}

int main(int argc, char* argv[]) {
  base::Init(argc, argv);
  log::InitTitLogging(argv[0], AddParentPrefix, nullptr);
  std::string cmd_line = base::CurrentCommandLine();
  base::ArgValueParser<int> int_parser;
  int handler = int_parser("handler");
  if (handler != (int) INTMAX_MAX) {
    log::InitTitLogging(argv[0], AddChildPrefix, nullptr);
    LOG(INFO) << "Born from parent";
    mojo::LibuvIOTaskRunnerAdapter* io_task_runner =
        new mojo::LibuvIOTaskRunnerAdapter();

    mojo::ChannelPosix* write_channel =
        new mojo::ChannelPosix(nullptr, io_task_runner, handler);
    write_channel->Start();
    mojo::ports::PortName port_name(123, 456);
    std::string data("hello world");
    mojo::ports::UserMessage::Ptr message =
        mojo::ports::UserMessage::Create(port_name, data);
    mojo::Protocol::Ptr protocol =
        mojo::Protocol::Create(
            mojo::Protocol::MsgType::kNormal,
            message->Encode()->toString());
    write_channel->Write(protocol);
    io_task_runner->Run();
    return 0;
  }

  int socket_pair[2];
  socketpair(AF_UNIX, SOCK_STREAM, 0, socket_pair);
  mojo::sock::set_nonblock(socket_pair[0]);
  mojo::sock::set_nonblock(socket_pair[1]);

  mojo::LibuvIOTaskRunnerAdapter* io_task_runner =
      new mojo::LibuvIOTaskRunnerAdapter();

  mojo::ChannelPosix* read_channel =
      new mojo::ChannelPosix(nullptr, io_task_runner, socket_pair[1]);
  read_channel->Start();

  std::string argument("-handler=");
  argument.append(std::to_string(socket_pair[0]));
  char* argument_list[] = {
      const_cast<char*>(base::CurrentExecuteName().data()),
      const_cast<char*>(argument.data()), NULL};

  Process* child_process = base::Process::LaunchProcess(argument_list);
  delete child_process;

  mojo::ports::PortName port_name(123, 456);
  std::string data("send from reader");
  mojo::ports::UserMessage::Ptr message =
      mojo::ports::UserMessage::Create(port_name, data);
  mojo::Protocol::Ptr protocol =
      mojo::Protocol::Create(
          mojo::Protocol::MsgType::kNormal,
          message->Encode()->toString());
  read_channel->Write(protocol);

  io_task_runner->Run();

  return 0;
}
