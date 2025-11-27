// SPDX-FileCopyrightText: (c) 2022 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <include/cef_load_handler.h>
#include <functional>

module pragma.modules.chromium.wrapper;

import :browser_load_handler;

cef::BrowserLoadHandler::BrowserLoadHandler() : CefLoadHandler {} {}
cef::BrowserLoadHandler::~BrowserLoadHandler() {}
void cef::BrowserLoadHandler::SetOnLoadingStateChange(std::function<void(bool, bool, bool)> onLoadingStateChange) { m_onLoadingStateChange = onLoadingStateChange; }
void cef::BrowserLoadHandler::SetOnLoadStart(std::function<void(TransitionType)> onLoadStart) { m_onLoadStart = onLoadStart; }
void cef::BrowserLoadHandler::SetOnLoadEnd(std::function<void(int)> onLoadEnd) { m_onLoadEnd = onLoadEnd; }
void cef::BrowserLoadHandler::SetOnLoadError(std::function<void(ErrorCode, const CefString &, const CefString &)> onLoadError) { m_onLoadError = onLoadError; }
void cef::BrowserLoadHandler::OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward)
{
	if(m_onLoadingStateChange)
		m_onLoadingStateChange(isLoading, canGoBack, canGoForward);
}
void cef::BrowserLoadHandler::OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, TransitionType transition_type)
{
	// Sending process messages here causes odd issues:
	// https://www.magpcss.org/ceforum/viewtopic.php?f=6&t=18848&p=50944
	// browser->GetMainFrame()->SendProcessMessage(PID_BROWSER,CefProcessMessage::Create("LoadStart"));
	// static_cast<WebBrowserClient*>(browser->GetHost()->GetClient().get())->SetPageLoadingStarted(true);
	if(m_onLoadStart)
		m_onLoadStart(transition_type);
}
void cef::BrowserLoadHandler::OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode)
{
	// if(httpStatusCode == 200)
	// 	browser->GetMainFrame()->SendProcessMessage(PID_BROWSER,CefProcessMessage::Create("LoadComplete"));
	// if(httpStatusCode == 200)
	// 	static_cast<WebBrowserClient*>(browser->GetHost()->GetClient().get())->SetPageLoadedSuccessfully(true);
	if(m_onLoadEnd)
		m_onLoadEnd(httpStatusCode);
}
void cef::BrowserLoadHandler::OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString &errorText, const CefString &failedUrl)
{
	if(m_onLoadError)
		m_onLoadError(errorCode, errorText, failedUrl);
}
