#pragma once

#include <memory>

#include "Hog/Core/PlatformDetection.h"

#ifdef HG_DEBUG
	#if defined(HG_PLATFORM_WINDOWS)
		#define HG_DEBUGBREAK() __debugbreak()
	#elif defined(HG_PLATFORM_LINUX)
		#include <signal.h>
		#define HG_DEBUGBREAK() raise(SIGTRAP)
	#else
		#error "Platform doesn't support debugbreak yet!"
	#endif
	#define HG_ENABLE_ASSERTS
#else
	#define HG_DEBUGBREAK()
#endif

#define HG_EXPAND_MACRO(x) x
#define HG_STRINGIFY_MACRO(x) #x

#define BIT(x) (1 << x)

#define HG_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }

namespace Hog {

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

#include "Hog/Core/Log.h"
#include "Hog/Core/Assert.h"
