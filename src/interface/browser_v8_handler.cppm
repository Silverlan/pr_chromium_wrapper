// SPDX-FileCopyrightText: (c) 2022 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <include/cef_v8.h>

export module pragma.modules.chromium.wrapper:browser_v8_handler;

export namespace cef {
	class BrowserV8Handler : public CefV8Handler {
	  public:
		BrowserV8Handler() = default;
		virtual bool Execute(const CefString &name, CefRefPtr<CefV8Value> object, const CefV8ValueList &arguments, CefRefPtr<CefV8Value> &retval, CefString &exception) override;
	  private:
		IMPLEMENT_REFCOUNTING(BrowserV8Handler);
	};
};
