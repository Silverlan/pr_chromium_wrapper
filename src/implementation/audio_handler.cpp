// SPDX-FileCopyrightText: (c) 2022 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <iostream>
#include <include/cef_audio_handler.h>
#include <include/cef_life_span_handler.h>

module pragma.modules.chromium.wrapper;

import :audio_handler;

cef::WebLifeSpanHandler::~WebLifeSpanHandler() {}
void cef::WebLifeSpanHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) { m_afterCreated = true; }
void cef::WebLifeSpanHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) { m_beforeClose = true; }

///

cef::WebAudioHandler::~WebAudioHandler() {}
bool cef::WebAudioHandler::GetAudioParameters(CefRefPtr<CefBrowser> browser, CefAudioParameters &params) { return true; }

void cef::WebAudioHandler::OnAudioStreamStarted(CefRefPtr<CefBrowser> browser, const CefAudioParameters &params, int channels) {}

void cef::WebAudioHandler::OnAudioStreamPacket(CefRefPtr<CefBrowser> browser, const float **data, int frames, int64_t pts) {}

void cef::WebAudioHandler::OnAudioStreamStopped(CefRefPtr<CefBrowser> browser) {}

void cef::WebAudioHandler::OnAudioStreamError(CefRefPtr<CefBrowser> browser, const CefString &message) {}
