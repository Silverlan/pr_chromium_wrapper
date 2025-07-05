// SPDX-FileCopyrightText: (c) 2022 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __PR_CHROMIUM_DOWNLOAD_HANDLER_HPP__
#define __PR_CHROMIUM_DOWNLOAD_HANDLER_HPP__

#include <include/cef_download_handler.h>
#include <functional>
#include <optional>

namespace cef {
	class WebDownloadHandler : public CefDownloadHandler {
	  public:
		struct DownloadInfo {
			enum class State : uint32_t { Downloading = 0, Cancelled, Complete, Invalidated };
			uint32_t id = 0;
			std::optional<std::string> name {};
			int32_t percentageComplete = 0;
			State state = State::Downloading;
		};
		virtual ~WebDownloadHandler() override;
		virtual bool OnBeforeDownload(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item, const CefString &suggested_name, CefRefPtr<CefBeforeDownloadCallback> callback) override;
		virtual void OnDownloadUpdated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item, CefRefPtr<CefDownloadItemCallback> callback) override;
		void SetDownloadLocation(std::string location);
		void SetStartCallback(const std::function<void(uint32_t, const std::string &)> &startCallback) { m_startCallback = startCallback; }
		void SetUpdateCallback(const std::function<void(uint32_t, DownloadInfo::State, int32_t)> &updateCallback) { m_updateCallback = updateCallback; }

		IMPLEMENT_REFCOUNTING(WebDownloadHandler);
	  private:
		void UpdateDownload(uint32_t id, std::optional<DownloadInfo::State> state, std::optional<uint32_t> percentage, std::string *name = nullptr);
		std::string m_downloadLocation;
		std::function<void(uint32_t, const std::string &)> m_startCallback = nullptr;
		std::function<void(uint32_t, DownloadInfo::State, int32_t)> m_updateCallback = nullptr;
		std::unordered_map<uint32_t, DownloadInfo> m_downloads;
	};
};

#endif
