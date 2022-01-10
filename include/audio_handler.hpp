#ifndef __PR_CHROMIUM_AUDIO_HANDLER_HPP__
#define __PR_CHROMIUM_AUDIO_HANDLER_HPP__

#include <include/cef_audio_handler.h>
#include <include/cef_life_span_handler.h>
#include <atomic>

namespace cef
{
	class WebLifeSpanHandler
		: public CefLifeSpanHandler
	{
	public:
		virtual ~WebLifeSpanHandler() override;
		virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
		virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;
		bool GetAfterCreated() const {return m_afterCreated;}
		bool GetBeforeClose() const {return m_beforeClose;}
		IMPLEMENT_REFCOUNTING(WebLifeSpanHandler);
	private:
		std::atomic<bool> m_afterCreated = false;
		std::atomic<bool> m_beforeClose = false;
	};
	class WebAudioHandler
		: public CefAudioHandler
	{
	public:
		virtual ~WebAudioHandler() override;
		virtual bool GetAudioParameters(CefRefPtr<CefBrowser> browser,CefAudioParameters& params) override;
		virtual void OnAudioStreamStarted(CefRefPtr<CefBrowser> browser,const CefAudioParameters& params,int channels) override;

		virtual void OnAudioStreamPacket(CefRefPtr<CefBrowser> browser,const float** data,int frames,int64 pts) override;

		virtual void OnAudioStreamStopped(CefRefPtr<CefBrowser> browser) override;

		virtual void OnAudioStreamError(CefRefPtr<CefBrowser> browser,const CefString& message) override;
		IMPLEMENT_REFCOUNTING(WebAudioHandler);
	};
};

#endif
