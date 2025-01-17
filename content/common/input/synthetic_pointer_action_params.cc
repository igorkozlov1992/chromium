// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/input/synthetic_pointer_action_params.h"

namespace content {

SyntheticPointerActionParams::SyntheticPointerActionParams()
    : pointer_action_type_(PointerActionType::NOT_INITIALIZED) {
  index_ = gesture_source_type != MOUSE_INPUT ? -1 : 0;
}

SyntheticPointerActionParams::SyntheticPointerActionParams(
    PointerActionType action_type,
    GestureSourceType source_type)
    : pointer_action_type_(action_type) {
  gesture_source_type = source_type;
  index_ = gesture_source_type != MOUSE_INPUT ? -1 : 0;
}

SyntheticPointerActionParams::SyntheticPointerActionParams(
    const SyntheticPointerActionParams& other)
    : SyntheticGestureParams(other),
      pointer_action_type_(other.pointer_action_type()) {
  switch (other.pointer_action_type()) {
    case PointerActionType::PRESS:
    case PointerActionType::MOVE:
      index_ = other.index();
      position_ = other.position();
      break;
    case PointerActionType::RELEASE:
    case PointerActionType::IDLE:
    case PointerActionType::NOT_INITIALIZED:
      index_ = other.index();
      break;
    case PointerActionType::FINISH:
      break;
  }
}

SyntheticPointerActionParams::~SyntheticPointerActionParams() {}

SyntheticGestureParams::GestureType
SyntheticPointerActionParams::GetGestureType() const {
  return POINTER_ACTION;
}

const SyntheticPointerActionParams* SyntheticPointerActionParams::Cast(
    const SyntheticGestureParams* gesture_params) {
  DCHECK(gesture_params);
  DCHECK_EQ(POINTER_ACTION, gesture_params->GetGestureType());
  return static_cast<const SyntheticPointerActionParams*>(gesture_params);
}

}  // namespace content
