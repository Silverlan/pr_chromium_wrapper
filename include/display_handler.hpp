// SPDX-FileCopyrightText: (c) 2022 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __PR_CHROMIUM_DISPLAY_HANDLER_HPP__
#define __PR_CHROMIUM_DISPLAY_HANDLER_HPP__

#include <include/cef_display_handler.h>
#include <functional>

namespace cef {
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

#endif
