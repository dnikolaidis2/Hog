#pragma once

#include "VulkanCore/Core/Base.h"

// This ignores all warnings raised inside External headers
#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)


namespace VulkanCore {

	class Log
	{
	public:
		static void Init();

		static Ref<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		static Ref<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
	private:
		static Ref<spdlog::logger> s_CoreLogger;
		static Ref<spdlog::logger> s_ClientLogger;
	};

}

// Core log macros
#define VKC_CORE_TRACE(...)    ::VulkanCore::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define VKC_CORE_INFO(...)     ::VulkanCore::Log::GetCoreLogger()->info(__VA_ARGS__)
#define VKC_CORE_WARN(...)     ::VulkanCore::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define VKC_CORE_ERROR(...)    ::VulkanCore::Log::GetCoreLogger()->error(__VA_ARGS__)
#define VKC_CORE_CRITICAL(...) ::VulkanCore::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define VKC_TRACE(...)         ::VulkanCore::Log::GetClientLogger()->trace(__VA_ARGS__)
#define VKC_INFO(...)          ::VulkanCore::Log::GetClientLogger()->info(__VA_ARGS__)
#define VKC_WARN(...)          ::VulkanCore::Log::GetClientLogger()->warn(__VA_ARGS__)
#define VKC_ERROR(...)         ::VulkanCore::Log::GetClientLogger()->error(__VA_ARGS__)
#define VKC_CRITICAL(...)      ::VulkanCore::Log::GetClientLogger()->critical(__VA_ARGS__)
