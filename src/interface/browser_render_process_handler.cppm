// SPDX-FileCopyrightText: (c) 2022 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <include/cef_base.h>
#include <include/cef_render_process_handler.h>
#include <iostream>

export module pragma.modules.chromium.wrapper:browser_render_process_handler;

export namespace cef {
	class BrowserRenderProcessHandler : public CefRenderProcessHandler {
	  public:
		virtual ~BrowserRenderProcessHandler() override;
		virtual void OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) override;
		virtual void OnWebKitInitialized() override;
		virtual CefRefPtr<CefLoadHandler> GetLoadHandler() override;
		virtual void OnUncaughtException(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context, CefRefPtr<CefV8Exception> exception, CefRefPtr<CefV8StackTrace> stackTrace) override;
		CefRefPtr<CefV8Handler> GetV8Handler();
		CefRefPtr<CefV8Context> GetContext();
	  private:
		CefRefPtr<CefV8Handler> m_v8Handler = nullptr;
		CefRefPtr<CefV8Context> m_context = nullptr;
		CefRefPtr<CefLoadHandler> m_loadHandler = nullptr;
		IMPLEMENT_REFCOUNTING(BrowserRenderProcessHandler);
	};
};
