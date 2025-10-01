// SPDX-FileCopyrightText: (c) 2022 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <iostream>
#include <include/cef_v8.h>

module pragma.modules.chromium.wrapper;

import :browser_v8_handler;
import :javascript;

static CefRefPtr<CefV8Value> js_to_cef_value(const cef::JSValue &jsVal)
{
	switch(jsVal.type) {
	case cef::JSValueType::Null:
		return CefV8Value::CreateNull();
	case cef::JSValueType::Bool:
		return CefV8Value::CreateBool(*static_cast<bool *>(jsVal.data));
	case cef::JSValueType::Int:
		return CefV8Value::CreateInt(*static_cast<int32_t *>(jsVal.data));
	case cef::JSValueType::Double:
		return CefV8Value::CreateDouble(*static_cast<double *>(jsVal.data));
	case cef::JSValueType::Date:
		return CefV8Value::CreateDate({});
	case cef::JSValueType::String:
		return CefV8Value::CreateString(*static_cast<std::string *>(jsVal.data));
	case cef::JSValueType::Object:
	case cef::JSValueType::Array:
	case cef::JSValueType::Function:
	case cef::JSValueType::Undefined:
	default:
		return CefV8Value::CreateUndefined();
	}
}

static cef::JSValue cef_to_js_value(CefV8Value &v8Val)
{
	cef::JSValue v {};
	if(v8Val.IsNull() == true) {
		v.type = cef::JSValueType::Null;
		v.data = nullptr;
	}
	else if(v8Val.IsBool() == true) {
		v.type = cef::JSValueType::Bool;
		v.data = new bool {v8Val.GetBoolValue()};
	}
	else if(v8Val.IsInt() == true) {
		v.type = cef::JSValueType::Int;
		v.data = new int32_t {v8Val.GetIntValue()};
	}
	else if(v8Val.IsDouble() == true) {
		v.type = cef::JSValueType::Double;
		v.data = new double {v8Val.GetDoubleValue()};
	}
	else if(v8Val.IsDate() == true) {
		v.type = cef::JSValueType::Date;
		v.data = nullptr;
	}
	else if(v8Val.IsString() == true) {
		v.type = cef::JSValueType::String;
		v.data = new std::string {v8Val.GetStringValue()};
	}
	else if(v8Val.IsObject() == true) {
		v.type = cef::JSValueType::Object;
		v.data = nullptr;
	}
	else if(v8Val.IsArray() == true) {
		v.type = cef::JSValueType::Array;
		v.data = nullptr;
	}
	else if(v8Val.IsFunction() == true) {
		v.type = cef::JSValueType::Function;
		v.data = nullptr;
	}
	else {
		v.type = cef::JSValueType::Undefined;
		v.data = nullptr;
	}
	return v;
}

namespace cef {
	extern std::vector<JavaScriptFunction> g_globalJavaScriptFunctions;
};
bool cef::BrowserV8Handler::Execute(const CefString &name, CefRefPtr<CefV8Value> object, const CefV8ValueList &arguments, CefRefPtr<CefV8Value> &retval, CefString &exception)
{
	for(auto &jsf : cef::g_globalJavaScriptFunctions) {
		if(name == jsf.name) {
			std::vector<cef::JSValue> jsArgs;
			jsArgs.reserve(arguments.size());
			for(auto &arg : arguments)
				jsArgs.push_back(cef_to_js_value(*arg));

			auto jsRetVal = jsf.callback(jsArgs.data(), jsArgs.size());
			if(jsRetVal != nullptr)
				retval = js_to_cef_value(*jsRetVal);
			for(auto &arg : jsArgs) {
				if(!arg.data)
					continue;
				switch(arg.type) {
				case cef::JSValueType::Bool:
					delete static_cast<bool *>(arg.data);
					break;
				case cef::JSValueType::Int:
					delete static_cast<int32_t *>(arg.data);
					break;
				case cef::JSValueType::Double:
					delete static_cast<double *>(arg.data);
					break;
				case cef::JSValueType::String:
					delete static_cast<std::string *>(arg.data);
					break;
				default:
					throw std::logic_error {"Chromium: Unexpected data value for JS value of type '" + std::to_string(static_cast<std::underlying_type_t<decltype(arg.type)>>(arg.type))};
					break;
				}
			}
			return true;
		}
	}
	return false;
}
