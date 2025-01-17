// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_DISPLAY_DISPLAY_ANIMATOR_CHROMEOS_H_
#define ASH_DISPLAY_DISPLAY_ANIMATOR_CHROMEOS_H_

#include <map>
#include <memory>

#include "ash/ash_export.h"
#include "ash/display/display_animator.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "ui/display/manager/chromeos/display_configurator.h"

namespace aura {
class Window;
}  // namespace aura

namespace ui {
class Layer;
}  // namespace ui

namespace ash {

// DisplayAnimatorChromeOS provides the visual effects for
// ui::DisplayConfigurator, such like fade-out/in during changing
// the display mode.
class ASH_EXPORT DisplayAnimatorChromeOS
    : public DisplayAnimator,
      public ui::DisplayConfigurator::Observer {
 public:
  DisplayAnimatorChromeOS();
  ~DisplayAnimatorChromeOS() override;

  // DisplayAnimator
  void StartFadeOutAnimation(base::Closure callback) override;
  void StartFadeInAnimation() override;

 protected:
  // ui::DisplayConfigurator::Observer overrides:
  void OnDisplayModeChanged(
      const ui::DisplayConfigurator::DisplayStateList& outputs) override;
  void OnDisplayModeChangeFailed(
      const ui::DisplayConfigurator::DisplayStateList& displays,
      ui::MultipleDisplayState failed_new_state) override;

 private:
  // Clears all hiding layers.  Note that in case that this method is called
  // during an animation, the method call will cancel all of the animations
  // and *not* call the registered callback.
  void ClearHidingLayers();

  std::map<aura::Window*, std::unique_ptr<ui::Layer>> hiding_layers_;
  std::unique_ptr<base::OneShotTimer> timer_;
  base::WeakPtrFactory<DisplayAnimatorChromeOS> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DisplayAnimatorChromeOS);
};

}  // namespace ash

#endif  // ASH_DISPLAY_DISPLAY_ANIMATOR_CHROMEOS_H_
