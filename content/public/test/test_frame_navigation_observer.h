// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_TEST_TEST_FRAME_NAVIGATION_OBSERVER_H_
#define CONTENT_TEST_TEST_FRAME_NAVIGATION_OBSERVER_H_

#include <memory>
#include <set>

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"

class GURL;

namespace content {
class RenderFrameHost;

// Helper for waiting until the navigation in a specific frame tree node (and
// all of its subframes) has completed loading.
class TestFrameNavigationObserver : public WebContentsObserver {
 public:
  // Create and register a new TestFrameNavigationObserver which will track
  // navigations performed in the frame tree node associated with |adapter|.
  // Note that RenderFrameHost associated with |frame| might be destroyed during
  // the navigation (e.g. if the content commits in a new renderer process).
  explicit TestFrameNavigationObserver(const ToRenderFrameHost& adapter);

  ~TestFrameNavigationObserver() override;

  // Runs a nested message loop and blocks until the full load has
  // completed.
  void Wait();

  // Runs a nested message loop and blocks until the navigation in the
  // associated FrameTreeNode has committed.
  void WaitForCommit();

 private:
  // WebContentsObserver
  void DidStartProvisionalLoadForFrame(RenderFrameHost* render_frame_host,
                                       const GURL& validated_url,
                                       bool is_error_page) override;
  void DidCommitProvisionalLoadForFrame(
      RenderFrameHost* render_frame_host,
      const GURL& url,
      ui::PageTransition transition_type) override;
  void DidStopLoading() override;

  // The id of the FrameTreeNode in which navigations are peformed.
  int frame_tree_node_id_;

  // If true the navigation has started.
  bool navigation_started_;

  // If true, the navigation has committed.
  bool has_committed_;

  // If true, this object is waiting for commit only, not for the full load
  // of the document.
  bool wait_for_commit_;

  // The MessageLoopRunner used to spin the message loop.
  scoped_refptr<MessageLoopRunner> message_loop_runner_;

  DISALLOW_COPY_AND_ASSIGN(TestFrameNavigationObserver);
};

}  // namespace content

#endif  // CONTENT_TEST_TEST_FRAME_NAVIGATION_OBSERVER_H_
