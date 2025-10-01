// SPDX-FileCopyrightText: (c) 2022 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <include/cef_client.h>
#include <include/cef_render_handler.h>
#include <include/cef_audio_handler.h>
#include <include/cef_download_handler.h>
#include <include/cef_life_span_handler.h>
#include <include/cef_display_handler.h>
#include <include/cef_load_handler.h>

export module pragma.modules.chromium.wrapper:browser_client;

export {
	namespace cef {
		class WebAudioHandler;
		class WebDownloadHandler;
		class WebLifeSpanHandler;
	};
	class WebRenderHandler;
	class WebBrowserClient : public CefClient {
	  public:
		WebBrowserClient(WebRenderHandler *renderHandler, cef::WebAudioHandler *audioHandler, cef::WebLifeSpanHandler *lifeSpanHandler, cef::WebDownloadHandler *dlHandler);
		virtual ~WebBrowserClient() override;
		virtual CefRefPtr<CefRenderHandler> GetRenderHandler() override;
		virtual CefRefPtr<CefAudioHandler> GetAudioHandler() override;
		virtual CefRefPtr<CefDownloadHandler> GetDownloadHandler() override;
		virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override;
		virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() override;
		virtual CefRefPtr<CefLoadHandler> GetLoadHandler() override;

		virtual bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process, CefRefPtr<CefProcessMessage> message) override;

		bool WasPageLoadedSuccessfully() const;
		bool HasPageLoadingStarted() const;

		void SetPageLoadedSuccessfully(bool b);
		void SetPageLoadingStarted(bool b);

		void SetUserData(void *userData) { m_userData = userData; }
		void *GetUserData() { return m_userData; }

		IMPLEMENT_REFCOUNTING(WebBrowserClient);
	  private:
		CefRefPtr<CefRenderHandler> m_renderHandler;
		CefRefPtr<CefAudioHandler> m_audioHandler;
		CefRefPtr<CefDownloadHandler> m_downloadHandler;
		CefRefPtr<CefLifeSpanHandler> m_lifeSpanHandler;
		CefRefPtr<CefDisplayHandler> m_displayHandler;
		CefRefPtr<CefLoadHandler> m_loadHandler;
		void *m_userData = nullptr;
		bool m_bPageLoadedSuccessfully = false;
		bool m_bPageLoadingStarted = false;
	};
};
