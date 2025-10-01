// SPDX-FileCopyrightText: (c) 2022 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

//#include "zygote_handler.hpp"

#include <cstring>
#include <include/cef_app.h>
#include <include/cef_parser.h>
#include <thread>

#if defined(__linux__)
#include <cstdlib> //for unsetenv
#endif
//#include <atlstr.h>

module pragma.modules.chromium.wrapper;

import :renderer;
import :audio_handler;
import :browser_client;
import :browser_load_handler;
import :browser_process;
import :display_handler;
import :download_handler;
import :javascript;

WebRenderHandler::WebRenderHandler(cef::BrowserProcess *process, void (*fGetRootScreenRect)(cef::CWebRenderHandler *, int &, int &, int &, int &), void (*fGetViewRect)(cef::CWebRenderHandler *, int &, int &, int &, int &),
  void (*fGetScreenPoint)(cef::CWebRenderHandler *, int, int, int &, int &))
    : m_process(process), m_fGetRootScreenRect {fGetRootScreenRect}, m_fGetViewRect {fGetViewRect}, m_fGetScreenPoint {fGetScreenPoint}
{
}
WebRenderHandler::~WebRenderHandler() {}
bool WebRenderHandler::GetRootScreenRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
{
	int x, y, w, h;
	m_fGetRootScreenRect(m_refPtr, x, y, w, h);
	rect = CefRect(x, y, w, h);
	return true;
}
void WebRenderHandler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect)
{
	int x, y, w, h;
	m_fGetViewRect(m_refPtr, x, y, w, h);
	rect = CefRect(x, y, w, h);
}
bool WebRenderHandler::GetScreenPoint(CefRefPtr<CefBrowser> browser, int viewX, int viewY, int &screenX, int &screenY)
{
	m_fGetScreenPoint(m_refPtr, viewX, viewY, screenX, screenY);
	return true;
}
bool WebRenderHandler::GetScreenInfo(CefRefPtr<CefBrowser> browser, CefScreenInfo &screen_info) { return false; }
void WebRenderHandler::OnPopupShow(CefRefPtr<CefBrowser> browser, bool show) {}
void WebRenderHandler::OnPopupSize(CefRefPtr<CefBrowser> browser, const CefRect &rect) {}
void WebRenderHandler::SetImageData(void *ptr, uint32_t w, uint32_t h)
{
	m_imageData.dataPtr = ptr;
	m_imageData.width = w;
	m_imageData.height = h;
}

void WebRenderHandler::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void *buffer, int width, int height)
{
	if(!m_imageData.dataPtr)
		return;
	if(m_imageData.width != width || m_imageData.height != height) {
		// HACK: After resizing the browser window, OnPaint for some reason sometimes reports an old width and height
		// despite GetViewRect having updated the size properly.
		// No clue why this happens, but when it does, we need to force reload the entire view, but
		// we can't do that in this callback.
		// Instead, we just set this flag and then call
		// WasResized() with a small delay.
		m_rendererSizeMismatch = true;
		return;
	}
	m_rendererSizeMismatch = false;
	auto host = browser->GetHost();
	auto *client = static_cast<WebBrowserClient *>(host->GetClient().get());
	auto bLoaded = (client != nullptr); // (client != nullptr && client->HasPageLoadingStarted()) ? true : false;
	if(bLoaded == false)
		return;
	auto *srcPtr = static_cast<const uint8_t *>(buffer);
	auto *dstPtr = static_cast<uint8_t *>(m_imageData.dataPtr);
	m_dirtyRects.reserve(m_dirtyRects.size() + dirtyRects.size());
	for(auto &rect : dirtyRects)
		m_dirtyRects.push_back({rect.x, rect.y, rect.width, rect.height});
	constexpr uint8_t szPerPixel = 4u;
	if(dirtyRects.size() == 1) {
		auto &r = dirtyRects[0];
		if(r.x == 0 && r.y == 0 && r.width == width && r.height == height) {
			memcpy(dstPtr, srcPtr, width * height * szPerPixel);
			return;
		}
	}
	for(auto &r : dirtyRects) {
		auto offset = r.y * width + r.x;
		for(auto i = decltype(r.height) {0}; i < r.height; ++i) {
			memcpy(dstPtr + offset * szPerPixel, srcPtr + offset * szPerPixel, r.width * szPerPixel);
			offset += width;
		}
	}
}

const std::vector<WebRenderHandler::Rect> &WebRenderHandler::GetDirtyRects() const { return m_dirtyRects; }
void WebRenderHandler::ClearDirtyRects() { m_dirtyRects.clear(); }
bool WebRenderHandler::IsRendererSizeMismatched() const { return m_rendererSizeMismatch; }
bool WebRenderHandler::StartDragging(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDragData> drag_data, DragOperationsMask allowed_ops, int x, int y) { return false; }
void WebRenderHandler::UpdateDragCursor(CefRefPtr<CefBrowser> browser, DragOperation operation) {}
void WebRenderHandler::OnScrollOffsetChanged(CefRefPtr<CefBrowser> browser, double x, double y) {}
void WebRenderHandler::OnImeCompositionRangeChanged(CefRefPtr<CefBrowser> browser, const CefRange &selected_range, const RectList &character_bounds) {}

static CefRefPtr<CefApp> g_process = nullptr;

#if defined(__linux__)

struct PRIMEEnvs {
	std::string renderOffloadEnv;
	std::string vkLayerEnv;
	std::string glxVendorLibName;
	//TODO: Check if AMD offloading causes any bugs.
};
static void unset_prime_env(PRIMEEnvs &envs)
{
	char *holderEnv = getenv("__NV_PRIME_RENDER_OFFLOAD");
	if(holderEnv)
		envs.renderOffloadEnv = std::string(holderEnv);
	holderEnv = getenv("__VK_LAYER_NV_optimus");
	if(holderEnv)
		envs.vkLayerEnv = std::string(holderEnv);
	holderEnv = getenv("__GLX_VENDOR_LIBRARY_NAME");
	if(holderEnv)
		envs.glxVendorLibName = std::string(holderEnv);
	unsetenv("__NV_PRIME_RENDER_OFFLOAD");
	unsetenv("__VK_LAYER_NV_optimus");
	unsetenv("__GLX_VENDOR_LIBRARY_NAME");
}
static void restore_prime_env(PRIMEEnvs &envs)
{
	if(!envs.renderOffloadEnv.empty())
		setenv("__NV_PRIME_RENDER_OFFLOAD", envs.renderOffloadEnv.c_str(), 1);
	if(!envs.vkLayerEnv.empty())
		setenv("__VK_LAYER_NV_optimus", envs.vkLayerEnv.c_str(), 1);
	if(!envs.glxVendorLibName.empty())
		setenv("__GLX_VENDOR_LIBRARY_NAME", envs.glxVendorLibName.c_str(), 1);
}

#endif

const char kProcessType[] = "type";
const char kRendererProcess[] = "renderer";
#if defined(OS_LINUX)
const char kZygoteProcess[] = "zygote";
#endif
enum class ProcessType : uint8_t {
	Browser = 0,
	Renderer,
	Other,
};
static ProcessType GetProcessType(const CefRefPtr<CefCommandLine> &command_line)
{
	// The command-line flag won't be specified for the browser process.
	if(!command_line->HasSwitch(kProcessType))
		return ProcessType::Browser;

	const std::string &process_type = command_line->GetSwitchValue(kProcessType);
	if(process_type == kRendererProcess)
		return ProcessType::Renderer;

#if defined(OS_LINUX)
	// On Linux the zygote process is used to spawn other process types. Since we
	// don't know what type of process it will be we give it the renderer app.
	if(process_type == kZygoteProcess)
		return ProcessType::Renderer;
#endif

	return ProcessType::Other;
}

static CefRefPtr<CefCommandLine> CreateCommandLine(const CefMainArgs &main_args)
{
	CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
#if defined(OS_WIN)
	command_line->InitFromString(::GetCommandLineW());
#else
	command_line->InitFromArgv(main_args.argc, main_args.argv);
#endif
	return command_line;
}

CefRefPtr<CefApp> CreateRendererProcessApp() { return nullptr; }

CefRefPtr<CefApp> CreateOtherProcessApp() { return nullptr; }

CefRefPtr<CefApp> CreateBrowserProcessApp(bool subProcess, bool disableGpu) { return new cef::BrowserProcess(subProcess, disableGpu); }

static std::string cef_exit_code_to_string(int exitCode)
{
	switch(exitCode) {
	case CEF_RESULT_CODE_NORMAL_EXIT:
		return "CEF_RESULT_CODE_NORMAL_EXIT";
	case CEF_RESULT_CODE_KILLED:
		return "CEF_RESULT_CODE_KILLED";
	case CEF_RESULT_CODE_HUNG:
		return "CEF_RESULT_CODE_HUNG";
	case CEF_RESULT_CODE_KILLED_BAD_MESSAGE:
		return "CEF_RESULT_CODE_KILLED_BAD_MESSAGE";
	case CEF_RESULT_CODE_GPU_DEAD_ON_ARRIVAL:
		return "CEF_RESULT_CODE_GPU_DEAD_ON_ARRIVAL";

	case CEF_RESULT_CODE_MISSING_DATA:
		return "CEF_RESULT_CODE_MISSING_DATA";
	case CEF_RESULT_CODE_UNSUPPORTED_PARAM:
		return "CEF_RESULT_CODE_UNSUPPORTED_PARAM";
	case CEF_RESULT_CODE_PROFILE_IN_USE:
		return "CEF_RESULT_CODE_PROFILE_IN_USE";
	case CEF_RESULT_CODE_PACK_EXTENSION_ERROR:
		return "CEF_RESULT_CODE_PACK_EXTENSION_ERROR";
	case CEF_RESULT_CODE_NORMAL_EXIT_PROCESS_NOTIFIED:
		return "CEF_RESULT_CODE_NORMAL_EXIT_PROCESS_NOTIFIED";
	case CEF_RESULT_CODE_INVALID_SANDBOX_STATE:
		return "CEF_RESULT_CODE_INVALID_SANDBOX_STATE";
	case CEF_RESULT_CODE_CLOUD_POLICY_ENROLLMENT_FAILED:
		return "CEF_RESULT_CODE_CLOUD_POLICY_ENROLLMENT_FAILED";
	case CEF_RESULT_CODE_GPU_EXIT_ON_CONTEXT_LOST:
		return "CEF_RESULT_CODE_GPU_EXIT_ON_CONTEXT_LOST";
	case CEF_RESULT_CODE_NORMAL_EXIT_PACK_EXTENSION_SUCCESS:
		return "CEF_RESULT_CODE_NORMAL_EXIT_PACK_EXTENSION_SUCCESS";
	case CEF_RESULT_CODE_SYSTEM_RESOURCE_EXHAUSTED:
		return "CEF_RESULT_CODE_SYSTEM_RESOURCE_EXHAUSTED";

	case CEF_RESULT_CODE_SANDBOX_FATAL_INTEGRITY:
		return "CEF_RESULT_CODE_SANDBOX_FATAL_INTEGRITY";
	case CEF_RESULT_CODE_SANDBOX_FATAL_DROPTOKEN:
		return "CEF_RESULT_CODE_SANDBOX_FATAL_DROPTOKEN";
	case CEF_RESULT_CODE_SANDBOX_FATAL_FLUSHANDLES:
		return "CEF_RESULT_CODE_SANDBOX_FATAL_FLUSHANDLES";
	case CEF_RESULT_CODE_SANDBOX_FATAL_CACHEDISABLE:
		return "CEF_RESULT_CODE_SANDBOX_FATAL_CACHEDISABLE";
	case CEF_RESULT_CODE_SANDBOX_FATAL_CLOSEHANDLES:
		return "CEF_RESULT_CODE_SANDBOX_FATAL_CLOSEHANDLES";
	case CEF_RESULT_CODE_SANDBOX_FATAL_MITIGATION:
		return "CEF_RESULT_CODE_SANDBOX_FATAL_MITIGATION";
	case CEF_RESULT_CODE_SANDBOX_FATAL_MEMORY_EXCEEDED:
		return "CEF_RESULT_CODE_SANDBOX_FATAL_MEMORY_EXCEEDED";
	case CEF_RESULT_CODE_SANDBOX_FATAL_WARMUP:
		return "CEF_RESULT_CODE_SANDBOX_FATAL_WARMUP";
	case CEF_RESULT_CODE_SANDBOX_FATAL_BROKER_SHUTDOWN_HUNG:
		return "CEF_RESULT_CODE_SANDBOX_FATAL_BROKER_SHUTDOWN_HUNG";
	default:
		break;
	}
	static_assert(CEF_RESULT_CODE_NUM_VALUES == 7016, "Update this list when new enum values have been added in CEF!");
	return "Unknown error (" + std::to_string(exitCode) + ")";
}

static bool initialize_chromium(bool subProcess, const char *pathToSubProcess, const char *cachePath, bool cpuRenderingOnly, std::string &outErr, int subprocessArgc = 0, char **subprocessArgv = nullptr)
{
	static auto initialized = false;
	if(initialized == true)
		return (g_process != nullptr) ? true : false;
	initialized = true;

#ifdef __linux__
	CefMainArgs args(subprocessArgc, subprocessArgv);
#else
	CefMainArgs args {};
#endif

	CefRefPtr<CefApp> app;
	CefRefPtr<CefCommandLine> command_line = CreateCommandLine(args);
	switch(GetProcessType(command_line)) {
	case ProcessType::Browser:
		app = CreateBrowserProcessApp(subProcess, cpuRenderingOnly);
		break;
	case ProcessType::Renderer:
		app = CreateRendererProcessApp();
		break;
	case ProcessType::Other:
		app = CreateOtherProcessApp();
		break;
	}
	g_process = app;

	if(subProcess) {
		//we're subprocess; passthrough.

		auto result = CefExecuteProcess(args, g_process, nullptr); // ???
		if(result >= 0) {
			outErr = "CefExecuteProcess failed with error code " + std::to_string(result) + "!";
			return false;
		}
		return true;
	}

#if defined(__linux__)
	// CEF doesn't like PRIME offloading, run without it.
	PRIMEEnvs envs;
	unset_prime_env(envs);

#endif
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
	// CefString(&settings.log_file).FromASCII("cache/chromium/cef_log.txt");
	// settings.log_severity = LOGSEVERITY_VERBOSE;
	if(CefInitialize(args, settings, g_process, nullptr) == false) {
#if defined(__linux__)
		restore_prime_env(envs);
#endif
		auto exitCode = CefGetExitCode();
		outErr = "CefInitialize failed: " + cef_exit_code_to_string(exitCode) + " (Exit code " + std::to_string(exitCode) + ")!";
		return false;
	}
#if defined(__linux__)
	//delete argsRaw;
	restore_prime_env(envs);
#endif
	return true;
}

static void close_chromium()
{
	g_process = nullptr;
	CefShutdown();
	//std::this_thread::sleep_for(std::chrono::seconds{1});
}

namespace cef {
	std::vector<cef::JavaScriptFunction> g_globalJavaScriptFunctions;
}

#ifdef __linux__
#define DLL_PR_CHROMIUM __attribute__((visibility("default")))
#else
#define DLL_PR_CHROMIUM __declspec(dllexport)
#endif

extern "C" {
DLL_PR_CHROMIUM void pr_chromium_register_javascript_function(const char *name, cef::JSValue *(*const fCallback)(cef::JSValue *, uint32_t))
{
	cef::g_globalJavaScriptFunctions.push_back({});
	auto &jsf = cef::g_globalJavaScriptFunctions.back();
	jsf.name = name;
	jsf.callback = fCallback;
}
};

namespace cef {
	enum class Modifier : uint32_t {
		None = 0,
		CapsLockOn = 1,
		ShiftDown = CapsLockOn << 1u,
		ControlDown = CapsLockOn << 2u,
		AltDown = CapsLockOn << 3u,
		LeftMouseButton = CapsLockOn << 4u,
		MiddleMouseButton = CapsLockOn << 5u,
		RightMouseButton = CapsLockOn << 6u,
		CommandDown = CapsLockOn << 7u,
		NumLockOn = CapsLockOn << 8u,
		IsKeyPad = CapsLockOn << 9u,
		IsLeft = CapsLockOn << 10u,
		IsRight = CapsLockOn << 11u,
		AltGrDown = CapsLockOn << 12u,
		IsRepeat = CapsLockOn << 13u
	};
};
#include <thread>
extern "C" {
DLL_PR_CHROMIUM bool pr_chromium_initialize(const char *pathToSubProcess, const char *cachePath, bool cpuRenderingOnly, std::string &outErr) { return initialize_chromium(false, pathToSubProcess, cachePath, cpuRenderingOnly, outErr); }
DLL_PR_CHROMIUM void pr_chromium_close() { close_chromium(); }
DLL_PR_CHROMIUM bool pr_chromium_subprocess(int subprocessArgc, char **subprocessArgv)
{
	std::string err;
	if(initialize_chromium(true, "", "", false, err, subprocessArgc, subprocessArgv) == false)
		return false;
	close_chromium();
	return true;
}
DLL_PR_CHROMIUM void pr_chromium_do_message_loop_work() { CefDoMessageLoopWork(); }
DLL_PR_CHROMIUM bool pr_chromium_parse_url(const char *url, void (*r)(void *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *), void *userData)
{
	CefURLParts parts;
	if(!CefParseURL(url, parts))
		return false;
	auto toAscii = [](cef_string_t &str) -> std::string { return CefString {str.str, str.length}.ToString(); };
	r(userData, toAscii(parts.host).c_str(), toAscii(parts.fragment).c_str(), toAscii(parts.password).c_str(), toAscii(parts.origin).c_str(), toAscii(parts.path).c_str(), toAscii(parts.port).c_str(), toAscii(parts.query).c_str(), toAscii(parts.scheme).c_str(), toAscii(parts.spec).c_str(),
	  toAscii(parts.username).c_str());
	return true;
}
DLL_PR_CHROMIUM cef::CWebRenderHandler *pr_chromium_render_handler_create(void (*fGetRootScreenRect)(cef::CWebRenderHandler *, int &, int &, int &, int &), void (*fGetViewRect)(cef::CWebRenderHandler *, int &, int &, int &, int &),
  void (*fGetScreenPoint)(cef::CWebRenderHandler *, int, int, int &, int &))
{
	auto renderHandler = CefRefPtr<WebRenderHandler> {new WebRenderHandler {static_cast<cef::BrowserProcess *>(g_process.get()), fGetRootScreenRect, fGetViewRect, fGetScreenPoint}};
	auto *p = new cef::CWebRenderHandler {renderHandler};
	renderHandler->SetRefPtr(p);
	return p;
}
DLL_PR_CHROMIUM void pr_chromium_render_handler_release(cef::CWebRenderHandler *renderHandler) { delete renderHandler; }
DLL_PR_CHROMIUM void pr_chromium_render_handler_set_user_data(cef::CWebRenderHandler *renderHandler, void *userData) { (*renderHandler)->SetUserData(userData); }
DLL_PR_CHROMIUM void *pr_chromium_render_handler_get_user_data(cef::CWebRenderHandler *renderHandler) { return (*renderHandler)->GetUserData(); }

DLL_PR_CHROMIUM cef::CWebBrowserClient *pr_chromium_browser_client_create(cef::CWebRenderHandler *renderHandler)
{
	cef::WebAudioHandler *audioHandler = nullptr;
	auto *lifeSpanHandler = new cef::WebLifeSpanHandler {};
	// audioHandler = new cef::WebAudioHandler{}; // Not yet fully implemented
	auto browserClient = CefRefPtr<WebBrowserClient> {new WebBrowserClient {renderHandler->get(), audioHandler, lifeSpanHandler, new cef::WebDownloadHandler {}}};
	return new cef::CWebBrowserClient {browserClient};
}
DLL_PR_CHROMIUM void pr_chromium_browser_client_release(cef::CWebBrowserClient *browserClient) { delete browserClient; }
DLL_PR_CHROMIUM void pr_chromium_browser_client_set_user_data(cef::CWebBrowserClient *browserClient, void *userData) { (*browserClient)->SetUserData(userData); }
DLL_PR_CHROMIUM void *pr_chromium_browser_client_get_user_data(cef::CWebBrowserClient *browserClient) { return (*browserClient)->GetUserData(); }
DLL_PR_CHROMIUM void pr_chromium_browser_client_set_download_start_callback(cef::CWebBrowserClient *browserClient, void (*onStart)(cef::CWebBrowserClient *, uint32_t, const char *))
{
	static_cast<cef::WebDownloadHandler *>((*browserClient)->GetDownloadHandler().get())->SetStartCallback([onStart, browserClient](uint32_t id, const std::string &fileName) { onStart(browserClient, id, fileName.c_str()); });
}
DLL_PR_CHROMIUM void pr_chromium_browser_client_set_download_update_callback(cef::CWebBrowserClient *browserClient, void (*onUpdate)(cef::CWebBrowserClient *, uint32_t, cef::WebDownloadHandler::DownloadInfo::State, int32_t))
{
	static_cast<cef::WebDownloadHandler *>((*browserClient)->GetDownloadHandler().get())->SetUpdateCallback([onUpdate, browserClient](uint32_t id, cef::WebDownloadHandler::DownloadInfo::State state, int32_t percentage) { onUpdate(browserClient, id, state, percentage); });
}
DLL_PR_CHROMIUM void pr_chromium_browser_client_set_download_location(cef::CWebBrowserClient *browserClient, const char *location) { static_cast<cef::WebDownloadHandler *>((*browserClient)->GetDownloadHandler().get())->SetDownloadLocation(location); }
DLL_PR_CHROMIUM void pr_chromium_browser_client_set_on_address_change_callback(cef::CWebBrowserClient *browserClient, void (*onAddressChange)(cef::CWebBrowserClient *, const char *))
{
	static_cast<cef::WebDisplayHandler *>((*browserClient)->GetDisplayHandler().get())->SetOnAddressChangeCallback([browserClient, onAddressChange](std::string addr) { onAddressChange(browserClient, addr.c_str()); });
}
DLL_PR_CHROMIUM void pr_chromium_browser_client_set_on_loading_state_change(cef::CWebBrowserClient *browserClient, void (*onLoadingStateChange)(cef::CWebBrowserClient *, bool, bool, bool))
{
	static_cast<cef::BrowserLoadHandler *>((*browserClient)->GetLoadHandler().get())->SetOnLoadingStateChange([browserClient, onLoadingStateChange](bool isLoading, bool canGoBack, bool canGoForward) { onLoadingStateChange(browserClient, isLoading, canGoBack, canGoForward); });
}
DLL_PR_CHROMIUM void pr_chromium_browser_client_set_on_load_start(cef::CWebBrowserClient *browserClient, void (*onLoadStart)(cef::CWebBrowserClient *, int))
{
	static_cast<cef::BrowserLoadHandler *>((*browserClient)->GetLoadHandler().get())->SetOnLoadStart([browserClient, onLoadStart](CefLoadHandler::TransitionType transitionType) { onLoadStart(browserClient, transitionType); });
}
DLL_PR_CHROMIUM void pr_chromium_browser_client_set_on_load_end(cef::CWebBrowserClient *browserClient, void (*onLoadEnd)(cef::CWebBrowserClient *, int))
{
	static_cast<cef::BrowserLoadHandler *>((*browserClient)->GetLoadHandler().get())->SetOnLoadStart([browserClient, onLoadEnd](int httpStatusCode) { onLoadEnd(browserClient, httpStatusCode); });
}
DLL_PR_CHROMIUM void pr_chromium_browser_client_set_on_load_error(cef::CWebBrowserClient *browserClient, void (*onLoadError)(cef::CWebBrowserClient *, int, const char *, const char *))
{
	static_cast<cef::BrowserLoadHandler *>((*browserClient)->GetLoadHandler().get())->SetOnLoadError([browserClient, onLoadError](CefLoadHandler::ErrorCode errorCode, const CefString &errorText, const CefString &failedUrl) {
		onLoadError(browserClient, errorCode, errorText.ToString().c_str(), failedUrl.ToString().c_str());
	});
}

DLL_PR_CHROMIUM cef::CWebBrowser *pr_chromium_browser_create(cef::CWebBrowserClient *browserClient, const char *initialUrl)
{
	CefWindowInfo windowInfo;
	windowInfo.SetAsWindowless(0);
	CefBrowserSettings browserSettings;
	auto browser = CefRefPtr<CefBrowser> {CefBrowserHost::CreateBrowserSync(windowInfo, *browserClient, initialUrl, browserSettings, nullptr, nullptr)};
	auto *pBrowser = new cef::CWebBrowser {browser};

	auto *client = static_cast<WebBrowserClient *>((*pBrowser)->GetHost()->GetClient().get());
	auto *lifeSpanHandler = static_cast<cef::WebLifeSpanHandler *>(client->GetLifeSpanHandler().get());
	while(lifeSpanHandler->GetAfterCreated() == false) // Ensure browser is ready
		CefDoMessageLoopWork();
	return pBrowser;
}
DLL_PR_CHROMIUM void pr_chromium_browser_release(cef::CWebBrowser *browser)
{
	//(*browser)->GetHost()->CloseBrowser(true);
	bool closed = false;
	while(!closed) {
		closed = (*browser)->GetHost()->TryCloseBrowser();
		CefDoMessageLoopWork();
	}

	//auto *client = static_cast<WebBrowserClient*>((*browser)->GetHost()->GetClient().get());
	//auto *lifeSpanHandler = static_cast<cef::WebLifeSpanHandler*>(client->GetLifeSpanHandler().get());
	//while(lifeSpanHandler->GetBeforeClose() == false) // Ensure browser is closed
	//	CefDoMessageLoopWork();
	delete browser;
}
DLL_PR_CHROMIUM void pr_chromium_browser_close(cef::CWebBrowser *browser) { (*browser)->GetHost()->CloseBrowser(true); }
DLL_PR_CHROMIUM bool pr_chromium_browser_try_close(cef::CWebBrowser *browser) { return (*browser)->GetHost()->TryCloseBrowser(); }
DLL_PR_CHROMIUM void *pr_chromium_browser_get_user_data(cef::CWebBrowser *browser) { return static_cast<WebBrowserClient *>((*browser)->GetHost()->GetClient().get())->GetUserData(); }
DLL_PR_CHROMIUM void pr_chromium_browser_was_resized(cef::CWebBrowser *browser)
{
	(*browser)->GetHost()->WasResized();
	(*browser)->GetHost()->Invalidate(PET_VIEW);
}
DLL_PR_CHROMIUM void pr_chromium_browser_invalidate(cef::CWebBrowser *browser) { (*browser)->GetHost()->Invalidate(PET_VIEW); }

DLL_PR_CHROMIUM void pr_chromium_render_handler_set_image_data(cef::CWebRenderHandler *renderHandler, void *ptr, uint32_t w, uint32_t h) { (*renderHandler)->SetImageData(ptr, w, h); }
DLL_PR_CHROMIUM void pr_chromium_render_handler_get_dirty_rects(cef::CWebRenderHandler *renderHandler, const std::tuple<int, int, int, int> **rectsPtr, uint32_t &numRects)
{
	auto &rects = (*renderHandler)->GetDirtyRects();
	numRects = rects.size();
	*rectsPtr = rects.data();
}
DLL_PR_CHROMIUM void pr_chromium_render_handler_clear_dirty_rects(cef::CWebRenderHandler *renderHandler) { (*renderHandler)->ClearDirtyRects(); }
DLL_PR_CHROMIUM bool pr_chromium_render_handler_is_renderer_size_mismatched(cef::CWebRenderHandler *renderHandler) { return (*renderHandler)->IsRendererSizeMismatched(); }
// Browser
DLL_PR_CHROMIUM void pr_chromium_browser_load_url(cef::CWebBrowser *browser, const char *url) { (*browser)->GetMainFrame()->LoadURL(url); }
DLL_PR_CHROMIUM bool pr_chromium_browser_can_go_back(cef::CWebBrowser *browser) { return (*browser)->CanGoBack(); }
DLL_PR_CHROMIUM bool pr_chromium_browser_can_go_forward(cef::CWebBrowser *browser) { return (*browser)->CanGoForward(); }
DLL_PR_CHROMIUM void pr_chromium_browser_go_back(cef::CWebBrowser *browser) { (*browser)->GoBack(); }
DLL_PR_CHROMIUM void pr_chromium_browser_go_forward(cef::CWebBrowser *browser) { (*browser)->GoForward(); }
DLL_PR_CHROMIUM bool pr_chromium_browser_has_document(cef::CWebBrowser *browser) { return (*browser)->HasDocument(); }
DLL_PR_CHROMIUM bool pr_chromium_browser_is_loading(cef::CWebBrowser *browser) { return (*browser)->IsLoading(); }
DLL_PR_CHROMIUM void pr_chromium_browser_reload(cef::CWebBrowser *browser) { return (*browser)->Reload(); }
DLL_PR_CHROMIUM void pr_chromium_browser_reload_ignore_cache(cef::CWebBrowser *browser) { return (*browser)->ReloadIgnoreCache(); }
DLL_PR_CHROMIUM void pr_chromium_browser_stop_load(cef::CWebBrowser *browser) { return (*browser)->StopLoad(); }
DLL_PR_CHROMIUM void pr_chromium_browser_copy(cef::CWebBrowser *browser) { return (*browser)->GetMainFrame()->Copy(); }
DLL_PR_CHROMIUM void pr_chromium_browser_cut(cef::CWebBrowser *browser) { return (*browser)->GetMainFrame()->Cut(); }
DLL_PR_CHROMIUM void pr_chromium_browser_delete(cef::CWebBrowser *browser) { return (*browser)->GetMainFrame()->Delete(); }
DLL_PR_CHROMIUM void pr_chromium_browser_paste(cef::CWebBrowser *browser) { return (*browser)->GetMainFrame()->Paste(); }
DLL_PR_CHROMIUM void pr_chromium_browser_redo(cef::CWebBrowser *browser) { return (*browser)->GetMainFrame()->Redo(); }
DLL_PR_CHROMIUM void pr_chromium_browser_select_all(cef::CWebBrowser *browser) { return (*browser)->GetMainFrame()->SelectAll(); }
DLL_PR_CHROMIUM void pr_chromium_browser_undo(cef::CWebBrowser *browser) { return (*browser)->GetMainFrame()->Undo(); }
DLL_PR_CHROMIUM void pr_chromium_browser_set_zoom_level(cef::CWebBrowser *browser, double zoomLevel) { return (*browser)->GetHost()->SetZoomLevel(zoomLevel); }
DLL_PR_CHROMIUM double pr_chromium_browser_get_zoom_level(cef::CWebBrowser *browser) { return (*browser)->GetHost()->GetZoomLevel(); }
DLL_PR_CHROMIUM void pr_chromium_browser_send_event_mouse_move(cef::CWebBrowser *browser, int x, int y, bool mouseLeave, cef::Modifier mods)
{
	CefMouseEvent ev {};
	ev.x = x;
	ev.y = y;
	ev.modifiers = static_cast<std::underlying_type_t<cef::Modifier>>(mods);
	// std::cout<<"Event Mouse Move: "<<x<<","<<y<<","<<ev.modifiers<<std::endl;
	(*browser)->GetHost()->SendMouseMoveEvent(ev, mouseLeave);
}
DLL_PR_CHROMIUM void pr_chromium_browser_send_event_mouse_click(cef::CWebBrowser *browser, int x, int y, char btType, bool mouseUp, int clickCount)
{
	CefMouseEvent ev {};
	ev.x = x;
	ev.y = y;
	CefBrowserHost::MouseButtonType cefBtType;
	switch(btType) {
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
	(*browser)->GetHost()->SendMouseClickEvent(ev, cefBtType, mouseUp, clickCount);
}
DLL_PR_CHROMIUM void pr_chromium_browser_send_event_key(cef::CWebBrowser *browser, char c, int systemKey, int nativeKeyCode, bool pressed, cef::Modifier mods)
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
DLL_PR_CHROMIUM void pr_chromium_browser_send_event_char(cef::CWebBrowser *browser, char c, cef::Modifier mods)
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
DLL_PR_CHROMIUM void pr_chromium_browser_send_event_mouse_wheel(cef::CWebBrowser *browser, int x, int y, float deltaX, float deltaY)
{
	CefMouseEvent ev {};
	ev.x = x;
	ev.y = y;
	(*browser)->GetHost()->SendMouseWheelEvent(ev, deltaX * 10.f, deltaY * 10.f);
}
DLL_PR_CHROMIUM void pr_chromium_browser_set_focus(cef::CWebBrowser *browser, bool focus) { (*browser)->GetHost()->SetFocus(focus); }
DLL_PR_CHROMIUM void pr_chromium_browser_execute_java_script(cef::CWebBrowser *browser, const char *js, const char *url)
{
	auto mainFrame = (*browser)->GetMainFrame();
	mainFrame->ExecuteJavaScript(js, url ? url : mainFrame->GetURL(), 0);
}
};
#pragma optimize("", on)
