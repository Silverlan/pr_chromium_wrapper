// SPDX-FileCopyrightText: (c) 2022 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <iostream>
#include <include/cef_client.h>
#include <include/cef_browser.h>
#include <include/cef_render_handler.h>
#include <include/cef_audio_handler.h>
#include <include/cef_download_handler.h>
#include <include/cef_life_span_handler.h>
#include <include/cef_display_handler.h>
#include <include/cef_load_handler.h>

module pragma.modules.chromium.wrapper;

import :audio_handler;
import :browser_client;
import :browser_load_handler;
import :display_handler;
import :download_handler;
import :renderer;

WebBrowserClient::WebBrowserClient(WebRenderHandler *renderHandler, cef::WebAudioHandler *audioHandler, cef::WebLifeSpanHandler *lifeSpanHandler, cef::WebDownloadHandler *dlHandler)
    : m_renderHandler {renderHandler}, m_audioHandler {audioHandler}, m_downloadHandler {dlHandler}, m_lifeSpanHandler {lifeSpanHandler}
{
	m_displayHandler = new cef::WebDisplayHandler {};
	m_loadHandler = new cef::BrowserLoadHandler {};
}
WebBrowserClient::~WebBrowserClient() {}
bool WebBrowserClient::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process, CefRefPtr<CefProcessMessage> message)
{
	auto msgName = message->GetName().ToString();
	std::cout << "Chromium Process Message: " << msgName << std::endl;
	// if(msgName == "LoadStart")
	// 	SetPageLoadingStarted(true);
	// else if(msgName == "LoadComplete")
	// 	SetPageLoadedSuccessfully(true);
	// else
	// 	return false;
	return false;
}
CefRefPtr<CefRenderHandler> WebBrowserClient::GetRenderHandler() { return m_renderHandler; }
CefRefPtr<CefAudioHandler> WebBrowserClient::GetAudioHandler() { return m_audioHandler; }
CefRefPtr<CefDownloadHandler> WebBrowserClient::GetDownloadHandler() { return m_downloadHandler; }
CefRefPtr<CefLifeSpanHandler> WebBrowserClient::GetLifeSpanHandler() { return m_lifeSpanHandler; }
CefRefPtr<CefDisplayHandler> WebBrowserClient::GetDisplayHandler() { return m_displayHandler; }
CefRefPtr<CefLoadHandler> WebBrowserClient::GetLoadHandler() { return m_loadHandler; }

bool WebBrowserClient::HasPageLoadingStarted() const { return m_bPageLoadingStarted; }
bool WebBrowserClient::WasPageLoadedSuccessfully() const { return m_bPageLoadedSuccessfully; }

void WebBrowserClient::SetPageLoadedSuccessfully(bool b) { m_bPageLoadedSuccessfully = b; }
void WebBrowserClient::SetPageLoadingStarted(bool b) { m_bPageLoadingStarted = b; }
