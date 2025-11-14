// SPDX-FileCopyrightText: (c) 2022 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <include/cef_display_handler.h>
#include <iostream>

module pragma.modules.chromium.wrapper;

import :display_handler;

void cef::WebDisplayHandler::OnAddressChange(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString &url)
{
	if(m_onAddressChange)
		m_onAddressChange(url.ToString());
}
bool cef::WebDisplayHandler::OnConsoleMessage(CefRefPtr<CefBrowser> browser, cef_log_severity_t level, const CefString &message, const CefString &source, int line)
{
	std::cout << "OnConsoleMessage: [Lv:" << level << "] " << message.ToString() << " [Src:" << source.ToString() << ":" << line << std::endl;
	return false;
}
void cef::WebDisplayHandler::SetOnAddressChangeCallback(std::function<void(std::string)> onAddressChange) { m_onAddressChange = onAddressChange; }
