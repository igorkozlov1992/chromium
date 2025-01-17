// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/runner/host/out_of_process_native_runner.h"

#include <stdint.h>

#include <utility>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/task_runner.h"
#include "services/service_manager/runner/common/client_util.h"
#include "services/service_manager/runner/host/child_process_host.h"

namespace service_manager {

OutOfProcessNativeRunner::OutOfProcessNativeRunner(
    const base::FilePath& service_path,
    base::TaskRunner* launch_process_runner,
    NativeRunnerDelegate* delegate)
    : launch_process_runner_(launch_process_runner),
      delegate_(delegate),
      service_path_(service_path) {}

OutOfProcessNativeRunner::~OutOfProcessNativeRunner() {
  if (child_process_host_ && !service_path_.empty())
    child_process_host_->Join();
}

mojom::ServicePtr OutOfProcessNativeRunner::Start(
    const Identity& target,
    bool start_sandboxed,
    const base::Callback<void(base::ProcessId)>& pid_available_callback,
    const base::Closure& app_completed_callback) {
  DCHECK(app_completed_callback_.is_null());
  app_completed_callback_ = app_completed_callback;

  child_process_host_.reset(new ChildProcessHost(
      launch_process_runner_, delegate_, start_sandboxed, target,
      service_path_));
  return child_process_host_->Start(
      target, pid_available_callback,
      base::Bind(&OutOfProcessNativeRunner::AppCompleted,
                 base::Unretained(this)));
}

void OutOfProcessNativeRunner::AppCompleted() {
  if (child_process_host_)
    child_process_host_->Join();
  child_process_host_.reset();
  // This object may be deleted by this callback.
  base::Closure app_completed_callback = app_completed_callback_;
  app_completed_callback_.Reset();
  if (!app_completed_callback.is_null())
    app_completed_callback.Run();
}

OutOfProcessNativeRunnerFactory::OutOfProcessNativeRunnerFactory(
    base::TaskRunner* launch_process_runner,
    NativeRunnerDelegate* delegate)
    : launch_process_runner_(launch_process_runner), delegate_(delegate) {}

OutOfProcessNativeRunnerFactory::~OutOfProcessNativeRunnerFactory() {}

std::unique_ptr<NativeRunner> OutOfProcessNativeRunnerFactory::Create(
    const base::FilePath& service_path) {
  return base::MakeUnique<OutOfProcessNativeRunner>(
      service_path, launch_process_runner_, delegate_);
}

}  // namespace service_manager
