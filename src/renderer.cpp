#include "renderer.hpp"
#include "browser_process.hpp"
#include "browser_render_process_handler.hpp"
#include "browser_load_handler.hpp"
#include "browserclient.hpp"
#include "display_handler.hpp"
#include <include/cef_parser.h>
//#include <atlstr.h>
#pragma optimize("",off)
WebRenderHandler::WebRenderHandler(
	cef::BrowserProcess *process,
	void(*fGetRootScreenRect)(cef::CWebRenderHandler*,int&,int&,int&,int&),
	void(*fGetViewRect)(cef::CWebRenderHandler*,int&,int&,int&,int&),
	void(*fGetScreenPoint)(cef::CWebRenderHandler*,int,int,int&,int&)
)
	: m_process(process),m_fGetRootScreenRect{fGetRootScreenRect},m_fGetViewRect{fGetViewRect},m_fGetScreenPoint{fGetScreenPoint}
{}
WebRenderHandler::~WebRenderHandler() {}
bool WebRenderHandler::GetRootScreenRect(CefRefPtr<CefBrowser> browser,CefRect &rect)
{
	int x,y,w,h;
	m_fGetRootScreenRect(m_refPtr,x,y,w,h);
	rect = CefRect(x,y,w,h);
	return true;
}
void WebRenderHandler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
{
	int x,y,w,h;
	m_fGetViewRect(m_refPtr,x,y,w,h);
	rect = CefRect(x,y,w,h);
}
bool WebRenderHandler::GetScreenPoint(CefRefPtr<CefBrowser> browser,int viewX,int viewY,int &screenX,int &screenY)
{
	m_fGetScreenPoint(m_refPtr,viewX,viewY,screenX,screenY);
	return true;
}
bool WebRenderHandler::GetScreenInfo(CefRefPtr<CefBrowser> browser,CefScreenInfo &screen_info)
{
	return false;
}
void WebRenderHandler::OnPopupShow(CefRefPtr<CefBrowser> browser,bool show)
{

}
void WebRenderHandler::OnPopupSize(CefRefPtr<CefBrowser> browser,const CefRect &rect)
{

}
void WebRenderHandler::SetImageData(void *ptr,uint32_t w,uint32_t h)
{
	m_imageData.dataPtr = ptr;
	m_imageData.width = w;
	m_imageData.height = h;
}
void WebRenderHandler::OnPaint(CefRefPtr<CefBrowser> browser,PaintElementType type,const RectList &dirtyRects,const void *buffer,int width,int height)
{
	if(!m_imageData.dataPtr || m_imageData.width != width || m_imageData.height != height)
		return;
	auto host = browser->GetHost();
	auto *client = static_cast<WebBrowserClient*>(host->GetClient().get());
	auto bLoaded = (client != nullptr); // (client != nullptr && client->HasPageLoadingStarted()) ? true : false;
	if(bLoaded == false)
		return;
	auto *srcPtr = static_cast<const uint8_t*>(buffer);
	auto *dstPtr = static_cast<uint8_t*>(m_imageData.dataPtr);
	constexpr uint8_t szPerPixel = 4u;
	if(dirtyRects.size() == 1)
	{
		auto &r = dirtyRects[0];
		if(r.x == 0 && r.y == 0 && r.width == width && r.height == height)
		{
			memcpy(dstPtr,srcPtr,width *height *szPerPixel);
			return;
		}
	}
	for(auto &r : dirtyRects)
	{
		auto offset = r.y *width +r.x;
		for(auto i=decltype(r.height){0};i<r.height;++i)
		{
			memcpy(dstPtr +offset *szPerPixel,srcPtr +offset *szPerPixel,r.width *szPerPixel);
			offset += width;
		}
	}
}
bool WebRenderHandler::StartDragging(CefRefPtr<CefBrowser> browser,CefRefPtr<CefDragData> drag_data,DragOperationsMask allowed_ops,int x,int y)
{
	return false;
}
void WebRenderHandler::UpdateDragCursor(CefRefPtr<CefBrowser> browser,DragOperation operation)
{

}
void WebRenderHandler::OnScrollOffsetChanged(CefRefPtr<CefBrowser> browser,double x,double y)
{

}
void WebRenderHandler::OnImeCompositionRangeChanged(CefRefPtr<CefBrowser> browser,const CefRange &selected_range,const RectList &character_bounds)
{

}

static CefRefPtr<cef::BrowserProcess> g_process = nullptr;
static bool initialize_chromium(bool subProcess,const char *pathToSubProcess,const char *cachePath)
{
	static auto initialized = false;
	if(initialized == true)
		return (g_process != nullptr) ? true : false;
	initialized = true;
	g_process = new cef::BrowserProcess();
	CefMainArgs args {};
	if(subProcess)
	{
		auto result = CefExecuteProcess(args,g_process,nullptr); // ???
		if(result > 0)
			return false;
		return true;
	}

	CefSettings settings {};
	settings.windowless_rendering_enabled = true;
	settings.multi_threaded_message_loop = false;
	settings.uncaught_exception_stack_size = 1; // Required for CefRenderProcessHandler::OnUncaughtException callback
	// CefString(&settings.user_agent).FromString("Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:72.0) Gecko/20100101 Firefox/72.0");
	// settings.user_agent
	// settings.user_agent_product
	CefString(&settings.cache_path).FromASCII(cachePath);
	CefString(&settings.browser_subprocess_path).FromASCII(pathToSubProcess);
	settings.no_sandbox = true;
	if(CefInitialize(args,settings,g_process,nullptr) == false)
	{
		g_process = nullptr;
		return false;
	}
	return true;
}
#include <thread>
static void close_chromium()
{
	g_process = nullptr;
	CefShutdown();
	//std::this_thread::sleep_for(std::chrono::seconds{1});
}

#include "util_javascript.hpp"
namespace cef
{
	std::vector<cef::JavaScriptFunction> g_globalJavaScriptFunctions;
}


#ifdef __linux__
#define DLL_PR_CHROMIUM __attribute__((visibility("default")))
#else
#define DLL_PR_CHROMIUM __declspec(dllexport)
#endif

extern "C"
{
	DLL_PR_CHROMIUM void pr_chromium_register_javascript_function(const char *name,cef::JSValue*(* const fCallback)(cef::JSValue*,uint32_t))
	{
		cef::g_globalJavaScriptFunctions.push_back({});
		auto &jsf = cef::g_globalJavaScriptFunctions.back();
		jsf.name = name;
		jsf.callback = fCallback;
	}
};

namespace cef
{
	enum class Modifier : uint32_t
	{
		None = 0,
		CapsLockOn = 1,
		ShiftDown = CapsLockOn<<1u,
		ControlDown = CapsLockOn<<2u,
		AltDown = CapsLockOn<<3u,
		LeftMouseButton = CapsLockOn<<4u,
		MiddleMouseButton = CapsLockOn<<5u,
		RightMouseButton = CapsLockOn<<6u,
		CommandDown = CapsLockOn<<7u,
		NumLockOn = CapsLockOn<<8u,
		IsKeyPad = CapsLockOn<<9u,
		IsLeft = CapsLockOn<<10u,
		IsRight = CapsLockOn<<11u,
		AltGrDown = CapsLockOn<<12u,
		IsRepeat = CapsLockOn<<13u
	};
};
#include <thread>
extern "C"
{
	DLL_PR_CHROMIUM bool pr_chromium_initialize(const char *pathToSubProcess,const char *cachePath)
	{
		return initialize_chromium(false,pathToSubProcess,cachePath);
	}
	DLL_PR_CHROMIUM void pr_chromium_close()
	{
		close_chromium();
	}
	DLL_PR_CHROMIUM bool pr_chromium_subprocess()
	{
		if(initialize_chromium(true,"","") == false)
			return false;
		close_chromium();
		return true;
	}
	DLL_PR_CHROMIUM void pr_chromium_do_message_loop_work()
	{
		CefDoMessageLoopWork();
	}
	DLL_PR_CHROMIUM bool pr_chromium_parse_url(const char *url,void(*r)(void*,const char*,const char*,const char*,const char*,const char*,const char*,const char*,const char*,const char*,const char*),void *userData)
	{
		CefURLParts parts;
		if(!CefParseURL(url,parts))
			return false;
		auto toAscii = [](cef_string_t &str) -> std::string {
			return CefString{str.str,str.length}.ToString();
		};
		r(
			userData,
			toAscii(parts.host).c_str(),
			toAscii(parts.fragment).c_str(),
			toAscii(parts.password).c_str(),
			toAscii(parts.origin).c_str(),
			toAscii(parts.path).c_str(),
			toAscii(parts.port).c_str(),
			toAscii(parts.query).c_str(),
			toAscii(parts.scheme).c_str(),
			toAscii(parts.spec).c_str(),
			toAscii(parts.username).c_str()
		);
		return true;
	}
	DLL_PR_CHROMIUM cef::CWebRenderHandler* pr_chromium_render_handler_create(
		void(*fGetRootScreenRect)(cef::CWebRenderHandler*,int&,int&,int&,int&),void(*fGetViewRect)(cef::CWebRenderHandler*,int&,int&,int&,int&),void(*fGetScreenPoint)(cef::CWebRenderHandler*,int,int,int&,int&)
	)
	{
		auto renderHandler = CefRefPtr<WebRenderHandler>{new WebRenderHandler{static_cast<cef::BrowserProcess*>(g_process.get()),fGetRootScreenRect,fGetViewRect,fGetScreenPoint}};
		auto *p = new cef::CWebRenderHandler{renderHandler};
		renderHandler->SetRefPtr(p);
		return p;
	}
	DLL_PR_CHROMIUM void pr_chromium_render_handler_release(cef::CWebRenderHandler *renderHandler)
	{
		delete renderHandler;
	}
	DLL_PR_CHROMIUM void pr_chromium_render_handler_set_user_data(cef::CWebRenderHandler *renderHandler,void *userData)
	{
		(*renderHandler)->SetUserData(userData);
	}
	DLL_PR_CHROMIUM void* pr_chromium_render_handler_get_user_data(cef::CWebRenderHandler *renderHandler)
	{
		return (*renderHandler)->GetUserData();
	}

	DLL_PR_CHROMIUM cef::CWebBrowserClient* pr_chromium_browser_client_create(cef::CWebRenderHandler *renderHandler)
	{
		cef::WebAudioHandler *audioHandler = nullptr;
		auto *lifeSpanHandler = new cef::WebLifeSpanHandler{};
		// audioHandler = new cef::WebAudioHandler{}; // Not yet fully implemented
		auto browserClient = CefRefPtr<WebBrowserClient>{new WebBrowserClient{renderHandler->get(),audioHandler,lifeSpanHandler,new cef::WebDownloadHandler{}}};
		return new cef::CWebBrowserClient{browserClient};
	}
	DLL_PR_CHROMIUM void pr_chromium_browser_client_release(cef::CWebBrowserClient *browserClient)
	{
		delete browserClient;
	}
	DLL_PR_CHROMIUM void pr_chromium_browser_client_set_user_data(cef::CWebBrowserClient *browserClient,void *userData)
	{
		(*browserClient)->SetUserData(userData);
	}
	DLL_PR_CHROMIUM void* pr_chromium_browser_client_get_user_data(cef::CWebBrowserClient *browserClient)
	{
		return (*browserClient)->GetUserData();
	}
	DLL_PR_CHROMIUM void pr_chromium_browser_client_set_download_start_callback(
		cef::CWebBrowserClient *browserClient,void(*onStart)(cef::CWebBrowserClient*,uint32_t,const char*)
	)
	{
		static_cast<cef::WebDownloadHandler*>((*browserClient)->GetDownloadHandler().get())->SetStartCallback([onStart,browserClient](uint32_t id,const std::string &fileName) {
			onStart(browserClient,id,fileName.c_str());
		});
	}
	DLL_PR_CHROMIUM void pr_chromium_browser_client_set_download_update_callback(
		cef::CWebBrowserClient *browserClient,void(*onUpdate)(cef::CWebBrowserClient*,uint32_t,cef::WebDownloadHandler::DownloadInfo::State,int32_t)
	)
	{
		static_cast<cef::WebDownloadHandler*>((*browserClient)->GetDownloadHandler().get())->SetUpdateCallback(
			[onUpdate,browserClient](uint32_t id,cef::WebDownloadHandler::DownloadInfo::State state,int32_t percentage) {
			onUpdate(browserClient,id,state,percentage);
		});
	}
	DLL_PR_CHROMIUM void pr_chromium_browser_client_set_download_location(cef::CWebBrowserClient *browserClient,const char *location)
	{
		static_cast<cef::WebDownloadHandler*>((*browserClient)->GetDownloadHandler().get())->SetDownloadLocation(location);
	}
	DLL_PR_CHROMIUM void pr_chromium_browser_client_set_on_address_change_callback(
		cef::CWebBrowserClient *browserClient,void(*onAddressChange)(cef::CWebBrowserClient*,const char*)
	)
	{
		static_cast<cef::WebDisplayHandler*>((*browserClient)->GetDisplayHandler().get())->SetOnAddressChangeCallback([browserClient,onAddressChange](std::string addr) {
			onAddressChange(browserClient,addr.c_str());
		});
	}
	DLL_PR_CHROMIUM void pr_chromium_browser_client_set_on_loading_state_change(
		cef::CWebBrowserClient *browserClient,void(*onLoadingStateChange)(cef::CWebBrowserClient*,bool,bool,bool)
	)
	{
		static_cast<cef::BrowserLoadHandler*>((*browserClient)->GetLoadHandler().get())->SetOnLoadingStateChange([browserClient,onLoadingStateChange](bool isLoading,bool canGoBack,bool canGoForward) {
			onLoadingStateChange(browserClient,isLoading,canGoBack,canGoForward);
		});
	}
	DLL_PR_CHROMIUM void pr_chromium_browser_client_set_on_load_start(
		cef::CWebBrowserClient *browserClient,void(*onLoadStart)(cef::CWebBrowserClient*,int)
	)
	{
		static_cast<cef::BrowserLoadHandler*>((*browserClient)->GetLoadHandler().get())->SetOnLoadStart([browserClient,onLoadStart](CefLoadHandler::TransitionType transitionType) {
			onLoadStart(browserClient,transitionType);
		});
	}
	DLL_PR_CHROMIUM void pr_chromium_browser_client_set_on_load_end(
		cef::CWebBrowserClient *browserClient,void(*onLoadEnd)(cef::CWebBrowserClient*,int)
	)
	{
		static_cast<cef::BrowserLoadHandler*>((*browserClient)->GetLoadHandler().get())->SetOnLoadStart([browserClient,onLoadEnd](int httpStatusCode) {
			onLoadEnd(browserClient,httpStatusCode);
		});
	}
	DLL_PR_CHROMIUM void pr_chromium_browser_client_set_on_load_error(
		cef::CWebBrowserClient *browserClient,void(*onLoadError)(cef::CWebBrowserClient*,int,const char*,const char*)
	)
	{
		static_cast<cef::BrowserLoadHandler*>((*browserClient)->GetLoadHandler().get())->SetOnLoadError([browserClient,onLoadError](CefLoadHandler::ErrorCode errorCode,const CefString& errorText,const CefString& failedUrl) {
			onLoadError(browserClient,errorCode,errorText.ToString().c_str(),failedUrl.ToString().c_str());
		});
	}

	DLL_PR_CHROMIUM cef::CWebBrowser* pr_chromium_browser_create(cef::CWebBrowserClient *browserClient,const char *initialUrl)
	{
		CefWindowInfo windowInfo;
        windowInfo.SetAsWindowless(0);
		CefBrowserSettings browserSettings;
		auto browser = CefRefPtr<CefBrowser>{CefBrowserHost::CreateBrowserSync(windowInfo,*browserClient,initialUrl,browserSettings,nullptr,nullptr)};
		auto *pBrowser = new cef::CWebBrowser{browser};

		auto *client = static_cast<WebBrowserClient*>((*pBrowser)->GetHost()->GetClient().get());
		auto *lifeSpanHandler = static_cast<cef::WebLifeSpanHandler*>(client->GetLifeSpanHandler().get());
		while(lifeSpanHandler->GetAfterCreated() == false) // Ensure browser is ready
			CefDoMessageLoopWork();
		return pBrowser;
	}
	DLL_PR_CHROMIUM void pr_chromium_browser_release(cef::CWebBrowser *browser)
	{
		//(*browser)->GetHost()->CloseBrowser(true);
		bool closed = false;
		while (!closed) {
			closed = (*browser)->GetHost()->TryCloseBrowser();
			CefDoMessageLoopWork();
		}

		//auto *client = static_cast<WebBrowserClient*>((*browser)->GetHost()->GetClient().get());
		//auto *lifeSpanHandler = static_cast<cef::WebLifeSpanHandler*>(client->GetLifeSpanHandler().get());
		//while(lifeSpanHandler->GetBeforeClose() == false) // Ensure browser is closed
		//	CefDoMessageLoopWork();
		delete browser;
	}
	DLL_PR_CHROMIUM void pr_chromium_browser_close(cef::CWebBrowser *browser)
	{
		(*browser)->GetHost()->CloseBrowser(true);
	}
	DLL_PR_CHROMIUM void* pr_chromium_browser_get_user_data(cef::CWebBrowser *browser)
	{
		return static_cast<WebBrowserClient*>((*browser)->GetHost()->GetClient().get())->GetUserData();
	}
	DLL_PR_CHROMIUM void pr_chromium_browser_was_resized(cef::CWebBrowser *browser)
	{
		(*browser)->GetHost()->WasResized();
	}

	DLL_PR_CHROMIUM void pr_chromium_render_handler_set_image_data(cef::CWebRenderHandler *renderHandler,void *ptr,uint32_t w,uint32_t h)
	{
		(*renderHandler)->SetImageData(ptr,w,h);
	}
	// Browser
	DLL_PR_CHROMIUM void pr_chromium_browser_load_url(cef::CWebBrowser *browser,const char *url) {(*browser)->GetMainFrame()->LoadURL(url);}
	DLL_PR_CHROMIUM bool pr_chromium_browser_can_go_back(cef::CWebBrowser *browser) {return (*browser)->CanGoBack();}
	DLL_PR_CHROMIUM bool pr_chromium_browser_can_go_forward(cef::CWebBrowser *browser) {return (*browser)->CanGoForward();}
	DLL_PR_CHROMIUM void pr_chromium_browser_go_back(cef::CWebBrowser *browser) {(*browser)->GoBack();}
	DLL_PR_CHROMIUM void pr_chromium_browser_go_forward(cef::CWebBrowser *browser) {(*browser)->GoForward();}
	DLL_PR_CHROMIUM bool pr_chromium_browser_has_document(cef::CWebBrowser *browser) {return (*browser)->HasDocument();}
	DLL_PR_CHROMIUM bool pr_chromium_browser_is_loading(cef::CWebBrowser *browser) {return (*browser)->IsLoading();}
	DLL_PR_CHROMIUM void pr_chromium_browser_reload(cef::CWebBrowser *browser) {return (*browser)->Reload();}
	DLL_PR_CHROMIUM void pr_chromium_browser_reload_ignore_cache(cef::CWebBrowser *browser) {return (*browser)->ReloadIgnoreCache();}
	DLL_PR_CHROMIUM void pr_chromium_browser_stop_load(cef::CWebBrowser *browser) {return (*browser)->StopLoad();}
	DLL_PR_CHROMIUM void pr_chromium_browser_copy(cef::CWebBrowser *browser) {return (*browser)->GetMainFrame()->Copy();}
	DLL_PR_CHROMIUM void pr_chromium_browser_cut(cef::CWebBrowser *browser) {return (*browser)->GetMainFrame()->Cut();}
	DLL_PR_CHROMIUM void pr_chromium_browser_delete(cef::CWebBrowser *browser) {return (*browser)->GetMainFrame()->Delete();}
	DLL_PR_CHROMIUM void pr_chromium_browser_paste(cef::CWebBrowser *browser) {return (*browser)->GetMainFrame()->Paste();}
	DLL_PR_CHROMIUM void pr_chromium_browser_redo(cef::CWebBrowser *browser) {return (*browser)->GetMainFrame()->Redo();}
	DLL_PR_CHROMIUM void pr_chromium_browser_select_all(cef::CWebBrowser *browser) {return (*browser)->GetMainFrame()->SelectAll();}
	DLL_PR_CHROMIUM void pr_chromium_browser_undo(cef::CWebBrowser *browser) {return (*browser)->GetMainFrame()->Undo();}
	DLL_PR_CHROMIUM void pr_chromium_browser_set_zoom_level(cef::CWebBrowser *browser,double zoomLevel) {return (*browser)->GetHost()->SetZoomLevel(zoomLevel);}
	DLL_PR_CHROMIUM double pr_chromium_browser_get_zoom_level(cef::CWebBrowser *browser) {return (*browser)->GetHost()->GetZoomLevel();}
	DLL_PR_CHROMIUM void pr_chromium_browser_send_event_mouse_move(cef::CWebBrowser *browser,int x,int y,bool mouseLeave,cef::Modifier mods)
	{
		CefMouseEvent ev {};
		ev.x = x;
		ev.y = y;
		ev.modifiers = static_cast<std::underlying_type_t<cef::Modifier>>(mods);
		// std::cout<<"Event Mouse Move: "<<x<<","<<y<<","<<ev.modifiers<<std::endl;
		(*browser)->GetHost()->SendMouseMoveEvent(ev,mouseLeave);
	}
	DLL_PR_CHROMIUM void pr_chromium_browser_send_event_mouse_click(cef::CWebBrowser *browser,int x,int y,char btType,bool mouseUp,int clickCount)
	{
		CefMouseEvent ev {};
		ev.x = x;
		ev.y = y;
		CefBrowserHost::MouseButtonType cefBtType;
		switch(btType)
		{
		case 'l':
			cefBtType = CefBrowserHost::MouseButtonType::MBT_LEFT;
			break;
		case 'r':
			cefBtType = CefBrowserHost::MouseButtonType::MBT_RIGHT;
			break;
		case 'm':
			cefBtType = CefBrowserHost::MouseButtonType::MBT_MIDDLE;
			break;
		default:
			return;
		}
		(*browser)->GetHost()->SendMouseClickEvent(ev,cefBtType,mouseUp,clickCount);
	}
	DLL_PR_CHROMIUM void pr_chromium_browser_send_event_key(cef::CWebBrowser *browser,char c,int systemKey,int nativeKeyCode,bool pressed,cef::Modifier mods)
	{
		CefKeyEvent ev {};
		ev.type = pressed ? cef_key_event_type_t::KEYEVENT_KEYDOWN : cef_key_event_type_t::KEYEVENT_KEYUP;
		ev.modifiers = static_cast<std::underlying_type_t<cef::Modifier>>(mods);
		ev.character = c;
		ev.native_key_code = nativeKeyCode;
		ev.windows_key_code = nativeKeyCode;
		ev.unmodified_character = systemKey;
		(*browser)->GetHost()->SendKeyEvent(ev);
	}
	DLL_PR_CHROMIUM void pr_chromium_browser_send_event_char(cef::CWebBrowser *browser,char c,cef::Modifier mods)
	{
		CefKeyEvent ev {};
		ev.type = cef_key_event_type_t::KEYEVENT_CHAR;
		ev.modifiers = static_cast<std::underlying_type_t<cef::Modifier>>(mods);
		ev.character = c;
		ev.native_key_code = c;
		ev.windows_key_code = c;
		ev.unmodified_character = c;
		(*browser)->GetHost()->SendKeyEvent(ev);
	}
	DLL_PR_CHROMIUM void pr_chromium_browser_send_event_mouse_wheel(cef::CWebBrowser *browser,int x,int y,float deltaX,float deltaY)
	{
		CefMouseEvent ev {};
		ev.x = x;
		ev.y = y;
		(*browser)->GetHost()->SendMouseWheelEvent(ev,deltaX *10.f,deltaY *10.f);
	}
	DLL_PR_CHROMIUM void pr_chromium_browser_set_focus(cef::CWebBrowser *browser,bool focus) {(*browser)->GetHost()->SetFocus(focus);}
	DLL_PR_CHROMIUM void pr_chromium_browser_execute_java_script(cef::CWebBrowser *browser,const char *js,const char *url)
	{
		auto mainFrame = (*browser)->GetMainFrame();
		mainFrame->ExecuteJavaScript(js,url ? url : mainFrame->GetURL(),0);
	}
};
#pragma optimize("",on)
