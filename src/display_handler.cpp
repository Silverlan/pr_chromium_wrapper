#include "display_handler.hpp"
#include <iostream>

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
