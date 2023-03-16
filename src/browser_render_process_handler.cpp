#include "browser_render_process_handler.hpp"
#include "browser_v8_handler.hpp"
#include "util_javascript.hpp"
#include "browser_load_handler.hpp"

namespace cef {
	extern std::vector<JavaScriptFunction> g_globalJavaScriptFunctions;
};

#include <iostream>
cef::BrowserRenderProcessHandler::~BrowserRenderProcessHandler() {}

void cef::BrowserRenderProcessHandler::OnUncaughtException(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context, CefRefPtr<CefV8Exception> exception, CefRefPtr<CefV8StackTrace> stackTrace)
{
	std::cout << "OnUncaughtException: " << exception->GetMessage().ToString() << std::endl;
}

CefRefPtr<CefV8Handler> cef::BrowserRenderProcessHandler::GetV8Handler() { return m_v8Handler; }
CefRefPtr<CefV8Context> cef::BrowserRenderProcessHandler::GetContext() { return m_context; }

void cef::BrowserRenderProcessHandler::OnWebKitInitialized() {}
CefRefPtr<CefLoadHandler> cef::BrowserRenderProcessHandler::GetLoadHandler()
{
	if(m_loadHandler == nullptr)
		m_loadHandler = new BrowserLoadHandler();
	return m_loadHandler;
}
void cef::BrowserRenderProcessHandler::OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context)
{
	m_context = context;

	auto object = context->GetGlobal();
	m_v8Handler = new BrowserV8Handler();
	for(auto &jsf : g_globalJavaScriptFunctions) {
		auto func = CefV8Value::CreateFunction(jsf.name, m_v8Handler);

		object->SetValue(jsf.name, func, V8_PROPERTY_ATTRIBUTE_NONE);
	}
}
