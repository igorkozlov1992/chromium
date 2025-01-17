// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_RUNNER_HOST_OUT_OF_PROCESS_NATIVE_RUNNER_H_
#define SERVICES_SERVICE_MANAGER_RUNNER_HOST_OUT_OF_PROCESS_NATIVE_RUNNER_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "services/service_manager/native_runner.h"

namespace base {
class TaskRunner;
}

namespace service_manager {

class ChildProcessHost;
class NativeRunnerDelegate;

// An implementation of |NativeRunner| that runs a given service executable
// in a separate, dedicated process.
class OutOfProcessNativeRunner : public NativeRunner {
 public:
  OutOfProcessNativeRunner(const base::FilePath& service_path,
                           base::TaskRunner* launch_process_runner,
                           NativeRunnerDelegate* delegate);
  ~OutOfProcessNativeRunner() override;

  // NativeRunner:
  mojom::ServicePtr Start(
      const Identity& identity,
      bool start_sandboxed,
      const base::Callback<void(base::ProcessId)>& pid_available_callback,
      const base::Closure& app_completed_callback) override;

 private:
  void AppCompleted();

  base::TaskRunner* const launch_process_runner_;
  NativeRunnerDelegate* delegate_;

  const base::FilePath service_path_;
  base::Closure app_completed_callback_;

  std::unique_ptr<ChildProcessHost> child_process_host_;

  DISALLOW_COPY_AND_ASSIGN(OutOfProcessNativeRunner);
};

class OutOfProcessNativeRunnerFactory : public NativeRunnerFactory {
 public:
  OutOfProcessNativeRunnerFactory(base::TaskRunner* launch_process_runner,
                                  NativeRunnerDelegate* delegate);
  ~OutOfProcessNativeRunnerFactory() override;

  std::unique_ptr<NativeRunner> Create(
      const base::FilePath& service_path) override;

 private:
  base::TaskRunner* const launch_process_runner_;
  NativeRunnerDelegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(OutOfProcessNativeRunnerFactory);
};

}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_RUNNER_HOST_OUT_OF_PROCESS_NATIVE_RUNNER_H_
