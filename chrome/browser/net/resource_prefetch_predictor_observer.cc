// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/resource_prefetch_predictor_observer.h"

#include <memory>
#include <string>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_request_info.h"
#include "net/url_request/url_request.h"
#include "url/gurl.h"

namespace content {
class WebContents;
}

using content::BrowserThread;
using predictors::ResourcePrefetchPredictor;
using URLRequestSummary =
    predictors::ResourcePrefetchPredictor::URLRequestSummary;

namespace {

// Enum for measuring statistics pertaining to observed request, responses and
// redirects.
enum RequestStats {
  REQUEST_STATS_TOTAL_RESPONSES = 0,
  REQUEST_STATS_TOTAL_PROCESSED_RESPONSES = 1,
  REQUEST_STATS_NO_RESOURCE_REQUEST_INFO = 2,  // Not recorded (never was).
  REQUEST_STATS_NO_RENDER_FRAME_ID_FROM_REQUEST_INFO = 3,  // Not recorded.
  REQUEST_STATS_MAX = 4,
};

// Specific to main frame requests.
enum MainFrameRequestStats {
  MAIN_FRAME_REQUEST_STATS_TOTAL_REQUESTS = 0,
  MAIN_FRAME_REQUEST_STATS_PROCESSED_REQUESTS = 1,
  MAIN_FRAME_REQUEST_STATS_TOTAL_REDIRECTS = 2,
  MAIN_FRAME_REQUEST_STATS_PROCESSED_REDIRECTS = 3,
  MAIN_FRAME_REQUEST_STATS_TOTAL_RESPONSES = 4,
  MAIN_FRAME_REQUEST_STATS_PROCESSED_RESPONSES = 5,
  MAIN_FRAME_REQUEST_STATS_MAX = 6,
};

void ReportRequestStats(RequestStats stat) {
  UMA_HISTOGRAM_ENUMERATION("ResourcePrefetchPredictor.RequestStats",
                            stat,
                            REQUEST_STATS_MAX);
}

void ReportMainFrameRequestStats(MainFrameRequestStats stat) {
  UMA_HISTOGRAM_ENUMERATION("ResourcePrefetchPredictor.MainFrameRequestStats",
                            stat,
                            MAIN_FRAME_REQUEST_STATS_MAX);
}

bool TryToFillNavigationID(
    predictors::NavigationID* navigation_id,
    const content::ResourceRequestInfo::WebContentsGetter& web_contents_getter,
    const GURL& main_frame_url,
    const base::TimeTicks& creation_time) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  content::WebContents* web_contents = web_contents_getter.Run();
  if (!web_contents)
    return false;
  *navigation_id =
      predictors::NavigationID(web_contents, main_frame_url, creation_time);
  return true;
}

}  // namespace

namespace chrome_browser_net {

ResourcePrefetchPredictorObserver::ResourcePrefetchPredictorObserver(
    ResourcePrefetchPredictor* predictor)
    : predictor_(predictor->AsWeakPtr()) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

ResourcePrefetchPredictorObserver::~ResourcePrefetchPredictorObserver() {
  CHECK(BrowserThread::CurrentlyOn(BrowserThread::UI) ||
        BrowserThread::CurrentlyOn(BrowserThread::IO));
}

void ResourcePrefetchPredictorObserver::OnRequestStarted(
    net::URLRequest* request,
    content::ResourceType resource_type,
    const content::ResourceRequestInfo::WebContentsGetter&
        web_contents_getter) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (resource_type == content::RESOURCE_TYPE_MAIN_FRAME)
    ReportMainFrameRequestStats(MAIN_FRAME_REQUEST_STATS_TOTAL_REQUESTS);

  if (!ResourcePrefetchPredictor::ShouldRecordRequest(request, resource_type))
    return;

  auto summary = base::MakeUnique<URLRequestSummary>();
  summary->resource_url = request->original_url();
  summary->resource_type = resource_type;

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&ResourcePrefetchPredictorObserver::OnRequestStartedOnUIThread,
                 base::Unretained(this), base::Passed(std::move(summary)),
                 web_contents_getter, request->first_party_for_cookies(),
                 request->creation_time()));

  if (resource_type == content::RESOURCE_TYPE_MAIN_FRAME)
    ReportMainFrameRequestStats(MAIN_FRAME_REQUEST_STATS_PROCESSED_REQUESTS);
}

void ResourcePrefetchPredictorObserver::OnRequestRedirected(
    net::URLRequest* request,
    const GURL& redirect_url,
    const content::ResourceRequestInfo::WebContentsGetter&
        web_contents_getter) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  const content::ResourceRequestInfo* request_info =
      content::ResourceRequestInfo::ForRequest(request);
  if (request_info &&
      request_info->GetResourceType() == content::RESOURCE_TYPE_MAIN_FRAME) {
    ReportMainFrameRequestStats(MAIN_FRAME_REQUEST_STATS_TOTAL_REDIRECTS);
  }

  if (!ResourcePrefetchPredictor::ShouldRecordRedirect(request))
    return;

  auto summary = base::MakeUnique<URLRequestSummary>();
  if (!ResourcePrefetchPredictor::URLRequestSummary::SummarizeResponse(
          *request, summary.get())) {
    return;
  }
  summary->redirect_url = redirect_url;

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(
          &ResourcePrefetchPredictorObserver::OnRequestRedirectedOnUIThread,
          base::Unretained(this), base::Passed(std::move(summary)),
          web_contents_getter, request->first_party_for_cookies(),
          request->creation_time()));

  if (request_info &&
      request_info->GetResourceType() == content::RESOURCE_TYPE_MAIN_FRAME) {
    ReportMainFrameRequestStats(MAIN_FRAME_REQUEST_STATS_PROCESSED_REDIRECTS);
  }
}

void ResourcePrefetchPredictorObserver::OnResponseStarted(
    net::URLRequest* request,
    const content::ResourceRequestInfo::WebContentsGetter&
        web_contents_getter) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  ReportRequestStats(REQUEST_STATS_TOTAL_RESPONSES);

  const content::ResourceRequestInfo* request_info =
      content::ResourceRequestInfo::ForRequest(request);
  if (request_info &&
      request_info->GetResourceType() == content::RESOURCE_TYPE_MAIN_FRAME) {
    ReportMainFrameRequestStats(MAIN_FRAME_REQUEST_STATS_TOTAL_RESPONSES);
  }

  if (!ResourcePrefetchPredictor::ShouldRecordResponse(request))
    return;
  auto summary = base::MakeUnique<URLRequestSummary>();
  if (!ResourcePrefetchPredictor::URLRequestSummary::SummarizeResponse(
          *request, summary.get())) {
    return;
  }

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(
          &ResourcePrefetchPredictorObserver::OnResponseStartedOnUIThread,
          base::Unretained(this), base::Passed(std::move(summary)),
          web_contents_getter, request->first_party_for_cookies(),
          request->creation_time()));

  ReportRequestStats(REQUEST_STATS_TOTAL_PROCESSED_RESPONSES);
  if (request_info &&
      request_info->GetResourceType() == content::RESOURCE_TYPE_MAIN_FRAME) {
    ReportMainFrameRequestStats(MAIN_FRAME_REQUEST_STATS_PROCESSED_RESPONSES);
  }
}

void ResourcePrefetchPredictorObserver::OnRequestStartedOnUIThread(
    std::unique_ptr<URLRequestSummary> summary,
    const content::ResourceRequestInfo::WebContentsGetter& web_contents_getter,
    const GURL& main_frame_url,
    const base::TimeTicks& creation_time) const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!TryToFillNavigationID(&summary->navigation_id, web_contents_getter,
                             main_frame_url, creation_time)) {
    return;
  }
  predictor_->RecordURLRequest(*summary);
}

void ResourcePrefetchPredictorObserver::OnRequestRedirectedOnUIThread(
    std::unique_ptr<URLRequestSummary> summary,
    const content::ResourceRequestInfo::WebContentsGetter& web_contents_getter,
    const GURL& main_frame_url,
    const base::TimeTicks& creation_time) const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!TryToFillNavigationID(&summary->navigation_id, web_contents_getter,
                             main_frame_url, creation_time)) {
    return;
  }
  predictor_->RecordURLRedirect(*summary);
}

void ResourcePrefetchPredictorObserver::OnResponseStartedOnUIThread(
    std::unique_ptr<URLRequestSummary> summary,
    const content::ResourceRequestInfo::WebContentsGetter& web_contents_getter,
    const GURL& main_frame_url,
    const base::TimeTicks& creation_time) const {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!TryToFillNavigationID(&summary->navigation_id, web_contents_getter,
                             main_frame_url, creation_time)) {
    return;
  }
  predictor_->RecordURLResponse(*summary);
}

}  // namespace chrome_browser_net
