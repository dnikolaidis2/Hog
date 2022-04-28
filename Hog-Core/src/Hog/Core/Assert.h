#pragma once

#include "Hog/Core/Base.h"
#include "Hog/Core/Log.h"
#include <filesystem>

#ifdef HG_ENABLE_ASSERTS

	// Alteratively we could use the same "default" message for both "WITH_MSG" and "NO_MSG" and
	// provide support for custom formatting by concatenating the formatting string instead of having the format inside the default message
	#define HG_INTERNAL_ASSERT_IMPL(type, check, msg, ...) { if(!(check)) { HG##type##ERROR(msg, __VA_ARGS__); HG_DEBUGBREAK(); } }
	#define HG_INTERNAL_ASSERT_WITH_MSG(type, check, ...) HG_INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
	#define HG_INTERNAL_ASSERT_NO_MSG(type, check) HG_INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", HG_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)

	#define HG_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
	#define HG_INTERNAL_ASSERT_GET_MACRO(...) HG_EXPAND_MACRO( HG_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, HG_INTERNAL_ASSERT_WITH_MSG, HG_INTERNAL_ASSERT_NO_MSG) )

	// Currently accepts at least the condition and one additional parameter (the message) being optional
	#define HG_ASSERT(...) HG_EXPAND_MACRO( HG_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__) )
	#define HG_CORE_ASSERT(...) HG_EXPAND_MACRO( HG_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_CORE_, __VA_ARGS__) )
#else
	#define HG_ASSERT(...)
	#define HG_CORE_ASSERT(...)
#endif
