// SPDX-FileCopyrightText: (c) 2022 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <include/cef_app.h>
#include <include/cef_render_process_handler.h>

export module pragma.modules.chromium.wrapper:browser_process;

export namespace cef {
	class BrowserProcess : public CefApp {
	  public:
		BrowserProcess(bool subProcess, bool disableGpu);
		virtual ~BrowserProcess() override;
		virtual CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override;
		virtual void OnBeforeCommandLineProcessing(const CefString &process_type, CefRefPtr<CefCommandLine> command_line) override;
	  private:
		IMPLEMENT_REFCOUNTING(BrowserProcess);
		CefRefPtr<CefRenderProcessHandler> m_renderProcessHandler = nullptr;
		bool m_subProcess;
		bool m_disableGpu;
	};
};
