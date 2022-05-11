//
// Created by titto on 2022/5/11.
//

#include "base/process.h"
#include "core/core.h"
#include "core/sock_pair_channel.h"
#include "core/invitation_dispatcher.h"
#include "core/libuv_io_task_runner_adapter.h"
#include "log/logging.h"

using namespace tit;
using namespace tit::mojo;

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

  Core* core = Core::Get();

  LibuvIOTaskRunnerAdapter* io_task_runner =
      new LibuvIOTaskRunnerAdapter();
  core->SetIOTaskRunner(io_task_runner);


  base::ArgValueParser<int> int_parser;
  int handler = int_parser("handler");
  if (handler != (int) INTMAX_MAX) {
    log::InitTitLogging(argv[0], AddChildPrefix, nullptr);
    MojoHandle invitee_handle;
    core->AcceptInvitation(&invitee_handle, handler);

    io_task_runner->Run();
    return 123;
  }

  SockPairChannel sock_pair;

  auto dispatcher = InvitationDispatcher::Create();
  MojoHandle invitation_handle = core->AddDispatcher(dispatcher);

  std::string argument("-handler=");
  argument.append(std::to_string(sock_pair.remote_endpoint()));
  char* argument_list[] = {
      const_cast<char*>(base::CurrentExecuteName().data()),
      const_cast<char*>(argument.data()), NULL};

  Process* child_process = base::Process::LaunchProcess(argument_list);
  delete child_process;

  core->SendInvitation(invitation_handle, sock_pair.local_endpoint());

  io_task_runner->Run();
  return 456;
}
