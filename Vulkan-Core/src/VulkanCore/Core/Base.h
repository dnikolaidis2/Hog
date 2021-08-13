#pragma once

#include <memory>

#include "VulkanCore/Core/PlatformDetection.h"

#ifdef VKC_DEBUG
	#if defined(VKC_PLATFORM_WINDOWS)
		#define VKC_DEBUGBREAK() __debugbreak()
	#elif defined(VKC_PLATFORM_LINUX)
		#include <signal.h>
		#define VKC_DEBUGBREAK() raise(SIGTRAP)
	#else
		#error "Platform doesn't support debugbreak yet!"
	#endif
	#define VKC_ENABLE_ASSERTS
#else
	#define VKC_DEBUGBREAK()
#endif

#define VKC_EXPAND_MACRO(x) x
#define VKC_STRINGIFY_MACRO(x) #x

#define BIT(x) (1 << x)

#define VKC_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

namespace VulkanCore {

	template<typename T>
	using Scope = std::unique_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Scope<T> CreateScope(Args&& ... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	using Ref = std::shared_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Ref<T> CreateRef(Args&& ... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

}

#include "VulkanCore/Core/Log.h"
#include "VulkanCore/Core/Assert.h"
