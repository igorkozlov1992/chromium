// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/display/manager/chromeos/test/action_logger.h"

namespace ui {
namespace test {

ActionLogger::ActionLogger() {}

ActionLogger::~ActionLogger() {}

void ActionLogger::AppendAction(const std::string& action) {
  if (!actions_.empty())
    actions_ += ",";
  actions_ += action;
}

std::string ActionLogger::GetActionsAndClear() {
  std::string actions = actions_;
  actions_.clear();
  return actions;
}

}  // namespace test
}  // namespace ui
