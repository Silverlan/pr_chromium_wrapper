#include "download_handler.hpp"
#include <iostream>

void cef::WebDownloadHandler::OnBeforeDownload(
	CefRefPtr<CefBrowser> browser,
	CefRefPtr<CefDownloadItem> download_item,
	const CefString& suggested_name,
	CefRefPtr<CefBeforeDownloadCallback> callback
)
{
	if(m_downloadLocation.empty())
	{
		callback->Continue("",false);
		auto fullPath = download_item->GetFullPath().ToString();
		UpdateDownload(download_item->GetId(),{},{},&fullPath);
	}
	else
	{
		auto filePath = m_downloadLocation +suggested_name.ToString();
		callback->Continue(filePath,false);
		UpdateDownload(download_item->GetId(),{},{},&filePath);
	}
}

void cef::WebDownloadHandler::OnDownloadUpdated(CefRefPtr<CefBrowser> browser,CefRefPtr<CefDownloadItem> download_item,CefRefPtr<CefDownloadItemCallback> callback)
{
	DownloadInfo::State state = DownloadInfo::State::Downloading;
	if(download_item->IsValid() == false)
		state = DownloadInfo::State::Invalidated;
	else if(download_item->IsComplete())
		state = DownloadInfo::State::Complete;
	else if(download_item->IsCanceled())
		state = DownloadInfo::State::Cancelled;

	UpdateDownload(download_item->GetId(),state,download_item->GetPercentComplete(),nullptr);
}
void cef::WebDownloadHandler::UpdateDownload(uint32_t id,std::optional<DownloadInfo::State> state,std::optional<uint32_t> percentage,std::string *name)
{
	auto it = m_downloads.find(id);
	if(it == m_downloads.end())
	{
		it = m_downloads.insert(std::make_pair(id,DownloadInfo{})).first;
		it->second.id = id;
	}
	if(state.has_value())
		it->second.state = *state;
	if(name)
	{
		it->second.name = *name;
		if(m_startCallback)
			m_startCallback(id,*it->second.name);
	}
	if(percentage.has_value())
		it->second.percentageComplete = *percentage;
	if(it->second.name.has_value() && m_updateCallback)
		m_updateCallback(id,it->second.state,it->second.percentageComplete);

	if(it->second.state == DownloadInfo::State::Cancelled || it->second.state == DownloadInfo::State::Invalidated)
		m_downloads.erase(it);
}
void cef::WebDownloadHandler::SetDownloadLocation(std::string location)
{
	for(auto &c : location)
	{
		if(c == '/')
			c = '\\';
	}
	m_downloadLocation = std::move(location);
	if(!m_downloadLocation.empty() && m_downloadLocation.back() != '\\')
		m_downloadLocation += '\\';
}
