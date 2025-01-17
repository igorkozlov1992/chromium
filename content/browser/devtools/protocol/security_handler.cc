// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/protocol/security_handler.h"

#include <string>

#include "content/browser/devtools/protocol/devtools_protocol_dispatcher.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/security_style_explanations.h"
#include "content/public/browser/ssl_status.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"

namespace content {
namespace devtools {
namespace security {

typedef DevToolsProtocolClient::Response Response;

namespace {

std::string SecurityStyleToProtocolSecurityState(
    blink::WebSecurityStyle security_style) {
  switch (security_style) {
    case blink::WebSecurityStyleUnknown:
      return kSecurityStateUnknown;
    case blink::WebSecurityStyleUnauthenticated:
      return kSecurityStateNeutral;
    case blink::WebSecurityStyleAuthenticationBroken:
      return kSecurityStateInsecure;
    case blink::WebSecurityStyleWarning:
      return kSecurityStateWarning;
    case blink::WebSecurityStyleAuthenticated:
      return kSecurityStateSecure;
    default:
      NOTREACHED();
      return kSecurityStateUnknown;
  }
}

void AddExplanations(
    const std::string& security_style,
    const std::vector<SecurityStyleExplanation>& explanations_to_add,
    std::vector<scoped_refptr<SecurityStateExplanation>>* explanations) {
  for (const auto& it : explanations_to_add) {
    scoped_refptr<SecurityStateExplanation> explanation =
        SecurityStateExplanation::Create()
            ->set_security_state(security_style)
            ->set_summary(it.summary)
            ->set_description(it.description)
            ->set_has_certificate(it.has_certificate);
    explanations->push_back(explanation);
  }
}

}  // namespace

SecurityHandler::SecurityHandler()
    : enabled_(false),
      host_(nullptr) {
}

SecurityHandler::~SecurityHandler() {
}

void SecurityHandler::SetClient(std::unique_ptr<Client> client) {
  client_.swap(client);
}

void SecurityHandler::AttachToRenderFrameHost() {
  DCHECK(host_);
  WebContents* web_contents = WebContents::FromRenderFrameHost(host_);
  WebContentsObserver::Observe(web_contents);

  // Send an initial DidChangeVisibleSecurityState event.
  DCHECK(enabled_);
  DidChangeVisibleSecurityState();
}

void SecurityHandler::SetRenderFrameHost(RenderFrameHost* host) {
  host_ = host;
  if (enabled_ && host_)
    AttachToRenderFrameHost();
}

void SecurityHandler::DidChangeVisibleSecurityState() {
  DCHECK(enabled_);

  SecurityStyleExplanations security_style_explanations;
  blink::WebSecurityStyle security_style =
      web_contents()->GetDelegate()->GetSecurityStyle(
          web_contents(), &security_style_explanations);

  const std::string security_state =
      SecurityStyleToProtocolSecurityState(security_style);

  std::vector<scoped_refptr<SecurityStateExplanation>> explanations;
  AddExplanations(kSecurityStateInsecure,
                  security_style_explanations.broken_explanations,
                  &explanations);
  AddExplanations(kSecurityStateNeutral,
                  security_style_explanations.unauthenticated_explanations,
                  &explanations);
  AddExplanations(kSecurityStateSecure,
                  security_style_explanations.secure_explanations,
                  &explanations);
  AddExplanations(kSecurityStateInfo,
                  security_style_explanations.info_explanations, &explanations);

  scoped_refptr<InsecureContentStatus> insecure_content_status =
      InsecureContentStatus::Create()
          ->set_ran_mixed_content(security_style_explanations.ran_mixed_content)
          ->set_displayed_mixed_content(
              security_style_explanations.displayed_mixed_content)
          ->set_ran_content_with_cert_errors(
              security_style_explanations.ran_content_with_cert_errors)
          ->set_displayed_content_with_cert_errors(
              security_style_explanations.displayed_content_with_cert_errors)
          ->set_ran_insecure_content_style(SecurityStyleToProtocolSecurityState(
              security_style_explanations.ran_insecure_content_style))
          ->set_displayed_insecure_content_style(
              SecurityStyleToProtocolSecurityState(
                  security_style_explanations
                      .displayed_insecure_content_style));

  client_->SecurityStateChanged(
      SecurityStateChangedParams::Create()
          ->set_security_state(security_state)
          ->set_scheme_is_cryptographic(
              security_style_explanations.scheme_is_cryptographic)
          ->set_insecure_content_status(insecure_content_status)
          ->set_explanations(explanations));
}

Response SecurityHandler::Enable() {
  enabled_ = true;
  if (host_)
    AttachToRenderFrameHost();

  return Response::OK();
}

Response SecurityHandler::Disable() {
  enabled_ = false;
  WebContentsObserver::Observe(nullptr);
  return Response::OK();
}

Response SecurityHandler::ShowCertificateViewer() {
  if (!host_)
    return Response::InternalError("Could not connect to view");
  WebContents* web_contents = WebContents::FromRenderFrameHost(host_);
  scoped_refptr<net::X509Certificate> certificate =
      web_contents->GetController().GetVisibleEntry()->GetSSL().certificate;
  if (!certificate)
    return Response::InternalError("Could not find certificate");
  web_contents->GetDelegate()->ShowCertificateViewerInDevTools(
      web_contents, certificate);
  return Response::OK();
}

}  // namespace security
}  // namespace devtools
}  // namespace content
