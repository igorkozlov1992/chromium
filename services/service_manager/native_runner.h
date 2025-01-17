// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_NATIVE_RUNNER_H_
#define SERVICES_SERVICE_MANAGER_NATIVE_RUNNER_H_

#include <memory>

#include "base/callback_forward.h"
#include "base/process/process_handle.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/service_manager/public/interfaces/service.mojom.h"

namespace base {
class FilePath;
}

namespace service_manager {
class Identity;

// Service Manager requires implementations of NativeRunner and
// NativeRunnerFactory to run native applications.
class NativeRunner {
 public:
  virtual ~NativeRunner() {}

  // Loads the app in the file at |service_path| and runs it on some other
  // thread/process. Returns a Service handle the service manager can use to
  // connect to the the app.
  virtual mojom::ServicePtr Start(
      const Identity& target,
      bool start_sandboxed,
      const base::Callback<void(base::ProcessId)>& pid_available_callback,
      const base::Closure& service_completed_callback) = 0;
};

class NativeRunnerFactory {
 public:
  virtual ~NativeRunnerFactory() {}
  virtual std::unique_ptr<NativeRunner> Create(
      const base::FilePath& service_path) = 0;
};

}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_NATIVE_RUNNER_H_
