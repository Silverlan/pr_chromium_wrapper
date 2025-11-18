// SPDX-FileCopyrightText: (c) 2022 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <include/cef_load_handler.h>

export module pragma.modules.chromium.wrapper:browser_load_handler;

import std;

export {
	class WebBrowserClient;
	namespace cef {
		class BrowserLoadHandler : public CefLoadHandler {
		  public:
			BrowserLoadHandler();
			virtual ~BrowserLoadHandler() override;
			virtual void OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward) override;
			virtual void OnLoadStart(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, TransitionType transition_type) override;
			virtual void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) override;
			virtual void OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString &errorText, const CefString &failedUrl) override;
			void SetOnLoadingStateChange(std::function<void(bool, bool, bool)> onLoadingStateChange);
			void SetOnLoadStart(std::function<void(TransitionType)> onLoadStart);
			void SetOnLoadEnd(std::function<void(int)> onLoadEnd);
			void SetOnLoadError(std::function<void(ErrorCode, const CefString &, const CefString &)> onLoadError);
		  private:
			IMPLEMENT_REFCOUNTING(BrowserLoadHandler);
			std::function<void(bool, bool, bool)> m_onLoadingStateChange = nullptr;
			std::function<void(TransitionType)> m_onLoadStart = nullptr;
			std::function<void(int)> m_onLoadEnd = nullptr;
			std::function<void(ErrorCode, const CefString &, const CefString &)> m_onLoadError = nullptr;
		};
	};
}
