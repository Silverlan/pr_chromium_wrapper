// SPDX-FileCopyrightText: (c) 2022 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#include "browser_process.hpp"
#include "browser_render_process_handler.hpp"

#include <iostream>

cef::BrowserProcess::BrowserProcess(bool subProcess) : m_subProcess{subProcess} {

    m_renderProcessHandler = new BrowserRenderProcessHandler();
}

cef::BrowserProcess::~BrowserProcess() {}

CefRefPtr<CefRenderProcessHandler> cef::BrowserProcess::GetRenderProcessHandler() {
    return m_renderProcessHandler;
}

void cef::BrowserProcess::OnBeforeCommandLineProcessing(const CefString &process_type, CefRefPtr<CefCommandLine> command_line)
{
    command_line->AppendSwitch("off-screen-rendering-enabled");
    command_line->AppendSwitch("no-zygote"); //for Linux?, this would disable zygote
    command_line->AppendSwitch("in-process-gpu");
    command_line->AppendSwitch("disable-gpu-sandbox");
    command_line->AppendSwitchWithValue("password-store","basic");

}
#pragma optimize("", on)
