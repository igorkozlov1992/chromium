// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_SILENT_SINK_SUSPENDER_H_
#define MEDIA_BASE_SILENT_SINK_SUSPENDER_H_

#include <stdint.h>

#include <deque>

#include "base/cancelable_callback.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/synchronization/lock.h"
#include "base/time/time.h"
#include "media/audio/fake_audio_worker.h"
#include "media/base/audio_parameters.h"
#include "media/base/audio_renderer_sink.h"
#include "media/base/media_export.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace media {

// Helper class for suspending AudioRenderSink instances after silence has been
// detected for some time. When this is detected, the provided |sink_| is paused
// and a fake sink is injected to black hole the silent audio data and avoid
// physical hardwasre usage. Note: The transition from real to fake audio output
// and vice versa may result in some irregular Render() callbacks.
class MEDIA_EXPORT SilentSinkSuspender
    : NON_EXPORTED_BASE(public AudioRendererSink::RenderCallback) {
 public:
  // |callback| is the true producer of audio data, |params| are the parameters
  // used to initialize |sink|, |sink| is the sink to monitor for idle, and
  // |worker| is the task runner to run the fake Render() callbacks on. The
  // amount of silence to allow before suspension is |silence_timeout|.
  SilentSinkSuspender(
      AudioRendererSink::RenderCallback* callback,
      base::TimeDelta silence_timeout,
      const AudioParameters& params,
      const scoped_refptr<AudioRendererSink>& sink,
      const scoped_refptr<base::SingleThreadTaskRunner>& worker);
  ~SilentSinkSuspender() override;

  // AudioRendererSink::RenderCallback implementation.
  int Render(base::TimeDelta delay,
             base::TimeTicks delay_timestamp,
             int prior_frames_skipped,
             AudioBus* dest) override;
  void OnRenderError() override;

  bool is_using_fake_sink_for_testing() const { return is_using_fake_sink_; }

 private:
  // If |use_fake_sink| is true, pauses |sink_| and plays |fake_sink_|; if
  // false, pauses |fake_sink_| and plays |sink_|.
  void TransitionSinks(bool use_fake_sink);

  // Actual RenderCallback providing audio data to the output device.
  AudioRendererSink::RenderCallback* const callback_;

  // Parameters used to construct |sink_|.
  const AudioParameters params_;

  // Sink monitored for silent output.
  scoped_refptr<AudioRendererSink> sink_;

  // Task runner this class is constructed on. Used to run TransitionSinks().
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  // Time when the silence starts.
  base::TimeTicks first_silence_time_;

  // Amount of time that can elapse before replacing |sink_| with |fake_sink_|.
  const base::TimeDelta silence_timeout_;

  // A fake audio sink object that consumes data when long period of silence
  // audio is detected. This object lives on |task_runner_| and will run
  // callbacks on RenderThreadImpl::GetMediaThreadTaskRunner().
  FakeAudioWorker fake_sink_;

  // AudioRendererSink::Pause() is not synchronous, so we need a lock to ensure
  // we don't have concurrent access to Render().
  base::Lock transition_lock_;

  // Whether audio output is directed to |fake_sink_|. Must only be used when
  // |transition_lock_| is held or both sinks are stopped.
  bool is_using_fake_sink_ = false;

  // Whether we're in the middle of a transition to or from |fake_sink_|. Must
  // only be used when |transition_lock_| is held or both sinks are stopped.
  bool is_transition_pending_ = false;

  // Buffers accumulated during the transition from |fake_sink_| to |sink_|.
  std::deque<std::unique_ptr<AudioBus>> buffers_after_silence_;

  // A cancelable task that is posted to switch to or from the |fake_sink_|
  // after a period of silence or first non-silent audio respective. We do this
  // on Android to save battery consumption.
  base::CancelableCallback<void(bool)> sink_transition_callback_;

  DISALLOW_COPY_AND_ASSIGN(SilentSinkSuspender);
};

}  // namespace content

#endif  // MEDIA_BASE_SILENT_SINK_SUSPENDER_H_
