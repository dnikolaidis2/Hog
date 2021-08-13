#pragma once

#include "VulkanCore/Core/Base.h"
#include "VulkanCore/Core/Log.h"
#include <filesystem>

#ifdef VKC_ENABLE_ASSERTS

	// Alteratively we could use the same "default" message for both "WITH_MSG" and "NO_MSG" and
	// provide support for custom formatting by concatenating the formatting string instead of having the format inside the default message
	#define VKC_INTERNAL_ASSERT_IMPL(type, check, msg, ...) { if(!(check)) { VKC##type##ERROR(msg, __VA_ARGS__); VKC_DEBUGBREAK(); } }
	#define VKC_INTERNAL_ASSERT_WITH_MSG(type, check, ...) VKC_INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
	#define VKC_INTERNAL_ASSERT_NO_MSG(type, check) VKC_INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", VKC_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)

	#define VKC_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
	#define VKC_INTERNAL_ASSERT_GET_MACRO(...) VKC_EXPAND_MACRO( VKC_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, VKC_INTERNAL_ASSERT_WITH_MSG, VKC_INTERNAL_ASSERT_NO_MSG) )

	// Currently accepts at least the condition and one additional parameter (the message) being optional
	#define VKC_ASSERT(...) VKC_EXPAND_MACRO( VKC_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__) )
	#define VKC_CORE_ASSERT(...) VKC_EXPAND_MACRO( VKC_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_CORE_, __VA_ARGS__) )
#else
	#define VKC_ASSERT(...)
	#define VKC_CORE_ASSERT(...)
#endif
