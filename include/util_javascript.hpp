// SPDX-FileCopyrightText: (c) 2022 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __UTIL_JAVASCRIPT_HPP__
#define __UTIL_JAVASCRIPT_HPP__

#include <string>
#include <functional>
#include <vector>
#include <cinttypes>
#include <memory>

namespace cef {
	enum class JSValueType : uint32_t { Undefined = 0, Null, Bool, Int, Double, Date, String, Object, Array, Function };

	struct JSValue {
		JSValueType type;
		void *data = nullptr;
	};

	struct JavaScriptFunction {
		std::string name;
		cef::JSValue *(*callback)(cef::JSValue *, uint32_t) = nullptr;
	};
};

#endif
