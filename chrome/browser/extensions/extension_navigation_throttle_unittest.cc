// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/stringprintf.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/crx_file/id_util.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/common/content_client.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "extensions/browser/extension_navigation_throttle.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/value_builder.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using content::NavigationThrottle;

namespace extensions {

namespace {

const char kAccessible[] = "accessible.html";
const char kPrivate[] = "private.html";
const char kAccessibleDir[] = "accessible_dir/*";
const char kAccessibleDirResource[] = "accessible_dir/foo.html";

class MockBrowserClient : public content::ContentBrowserClient {
 public:
  MockBrowserClient() {}
  ~MockBrowserClient() override {}

  // Only construct an ExtensionNavigationThrottle so that we can test it in
  // isolation.
  ScopedVector<NavigationThrottle> CreateThrottlesForNavigation(
      content::NavigationHandle* handle) override {
    ScopedVector<NavigationThrottle> throttles;
    if (!handle->IsInMainFrame())  // Mirrors ChromeContentBrowserClient.
      throttles.push_back(new ExtensionNavigationThrottle(handle));
    return throttles;
  }
};

}  // namespace

class ExtensionNavigationThrottleUnitTest
    : public ChromeRenderViewHostTestHarness {
 public:
  ExtensionNavigationThrottleUnitTest() {}
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    original_client_ = content::SetBrowserClientForTesting(&client_);
    AddExtension();
  }

  void TearDown() override {
    content::SetBrowserClientForTesting(original_client_);
    ChromeRenderViewHostTestHarness::TearDown();
  }

  // Checks that trying to navigate the given |host| to |url| results in the
  // |expected_result|.
  void CheckTestCase(content::RenderFrameHost* host,
                     const GURL& url,
                     NavigationThrottle::ThrottleCheckResult expected_result) {
    std::unique_ptr<content::NavigationHandle> handle =
        content::NavigationHandle::CreateNavigationHandleForTesting(url, host);
    EXPECT_EQ(expected_result,
              handle->CallWillStartRequestForTesting(
                  false,  // not post
                  content::Referrer(), ui::PAGE_TRANSITION_LINK, false))
        << url;
  }

  const Extension* extension() { return extension_.get(); }
  content::WebContentsTester* web_contents_tester() {
    return content::WebContentsTester::For(web_contents());
  }
  content::RenderFrameHostTester* render_frame_host_tester(
      content::RenderFrameHost* host) {
    return content::RenderFrameHostTester::For(host);
  }

 private:
  // Constructs an extension with accessible.html and accessible_dir/* as
  // accessible resources.
  void AddExtension() {
    DictionaryBuilder manifest;
    manifest.Set("name", "ext")
        .Set("description", "something")
        .Set("version", "0.1")
        .Set("manifest_version", 2)
        .Set("web_accessible_resources",
             ListBuilder().Append(kAccessible).Append(kAccessibleDir).Build());
    extension_ = ExtensionBuilder()
                     .SetManifest(manifest.Build())
                     .SetID(crx_file::id_util::GenerateId("foo"))
                     .Build();
    ASSERT_TRUE(extension_);
    ExtensionRegistry::Get(browser_context())->AddEnabled(extension_.get());
  }

  scoped_refptr<const Extension> extension_;
  MockBrowserClient client_;
  content::ContentBrowserClient* original_client_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionNavigationThrottleUnitTest);
};

// Tests the basic case of an external web page embedding an extension resource.
TEST_F(ExtensionNavigationThrottleUnitTest, ExternalWebPage) {
  web_contents_tester()->NavigateAndCommit(GURL("http://example.com"));
  content::RenderFrameHost* child =
      render_frame_host_tester(main_rfh())->AppendChild("child");

  // Only resources specified in web_accessible_resources should be allowed.
  CheckTestCase(child, extension()->GetResourceURL(kPrivate),
                NavigationThrottle::BLOCK_REQUEST);
  CheckTestCase(child, extension()->GetResourceURL(kAccessible),
                NavigationThrottle::PROCEED);
  CheckTestCase(child, extension()->GetResourceURL(kAccessibleDirResource),
                NavigationThrottle::PROCEED);
}

// Tests that the owning extension can access any of its resources.
TEST_F(ExtensionNavigationThrottleUnitTest, SameExtension) {
  web_contents_tester()->NavigateAndCommit(
      extension()->GetResourceURL("trusted.html"));
  content::RenderFrameHost* child =
      render_frame_host_tester(main_rfh())->AppendChild("child");

  // All resources should be allowed.
  CheckTestCase(child, extension()->GetResourceURL(kPrivate),
                NavigationThrottle::PROCEED);
  CheckTestCase(child, extension()->GetResourceURL(kAccessible),
                NavigationThrottle::PROCEED);
  CheckTestCase(child, extension()->GetResourceURL(kAccessibleDirResource),
                NavigationThrottle::PROCEED);
}

// Tests that if any of the ancestors are an external web page, we restrict
// the resources.
TEST_F(ExtensionNavigationThrottleUnitTest, WebPageAncestor) {
  web_contents_tester()->NavigateAndCommit(GURL("http://example.com"));
  content::RenderFrameHost* child =
      render_frame_host_tester(main_rfh())->AppendChild("subframe1");
  GURL url = extension()->GetResourceURL(kAccessible);
  render_frame_host_tester(child)->SimulateNavigationStart(url);
  render_frame_host_tester(child)->SimulateNavigationCommit(url);
  content::RenderFrameHost* grand_child =
      render_frame_host_tester(child)->AppendChild("grandchild");

  // Even though the immediate parent is a trusted frame, we should restrict
  // to web_accessible_resources since the grand parent is external.
  CheckTestCase(grand_child, extension()->GetResourceURL(kPrivate),
                NavigationThrottle::BLOCK_REQUEST);
  CheckTestCase(grand_child, extension()->GetResourceURL(kAccessible),
                NavigationThrottle::PROCEED);
  CheckTestCase(grand_child,
                extension()->GetResourceURL(kAccessibleDirResource),
                NavigationThrottle::PROCEED);
}

// Tests that requests to disabled or non-existent extensions are blocked.
TEST_F(ExtensionNavigationThrottleUnitTest, InvalidExtension) {
  web_contents_tester()->NavigateAndCommit(GURL("http://example.com"));
  content::RenderFrameHost* child =
      render_frame_host_tester(main_rfh())->AppendChild("child");

  ExtensionRegistry* registry = ExtensionRegistry::Get(browser_context());
  registry->RemoveEnabled(extension()->id());
  registry->AddDisabled(extension());

  // Since the extension is disabled, all requests should be blocked.
  CheckTestCase(child, extension()->GetResourceURL(kPrivate),
                NavigationThrottle::BLOCK_REQUEST);
  CheckTestCase(child, extension()->GetResourceURL(kAccessible),
                NavigationThrottle::BLOCK_REQUEST);
  CheckTestCase(child, extension()->GetResourceURL(kAccessibleDirResource),
                NavigationThrottle::BLOCK_REQUEST);

  std::string second_id = crx_file::id_util::GenerateId("bar");
  ASSERT_NE(second_id, extension()->id());
  GURL invalid_url(base::StringPrintf("chrome-extension://%s/accessible.html",
                                      second_id.c_str()));
  // Requests to non-existent extensions should be blocked.
  CheckTestCase(child, invalid_url, NavigationThrottle::BLOCK_REQUEST);
}

}  // namespace extensions
