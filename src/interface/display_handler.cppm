// SPDX-FileCopyrightText: (c) 2022 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <include/cef_display_handler.h>

export module pragma.modules.chromium.wrapper:display_handler;

import std;

export namespace cef {
	class WebDisplayHandler : public CefDisplayHandler {
	  public:
		virtual ~WebDisplayHandler() override {}
		virtual void OnAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString &url) override;
		virtual bool OnConsoleMessage(CefRefPtr<CefBrowser> browser, cef_log_severity_t level, const CefString &message, const CefString &source, int line) override;
		void SetOnAddressChangeCallback(std::function<void(std::string)> onAddressChange);
		IMPLEMENT_REFCOUNTING(WebDisplayHandler);
	  private:
		std::function<void(std::string)> m_onAddressChange = nullptr;
	};
};
