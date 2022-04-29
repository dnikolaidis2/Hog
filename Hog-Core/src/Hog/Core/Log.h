#pragma once

#include "Hog/Core/Base.h"

// This ignores all warnings raised inside External headers
#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)


namespace Hog {

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
#define HG_CORE_TRACE(...)    ::Hog::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define HG_CORE_INFO(...)     ::Hog::Log::GetCoreLogger()->info(__VA_ARGS__)
#define HG_CORE_WARN(...)     ::Hog::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define HG_CORE_ERROR(...)    ::Hog::Log::GetCoreLogger()->error(__VA_ARGS__)
#define HG_CORE_CRITICAL(...) ::Hog::Log::GetCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define HG_TRACE(...)         ::Hog::Log::GetClientLogger()->trace(__VA_ARGS__)
#define HG_INFO(...)          ::Hog::Log::GetClientLogger()->info(__VA_ARGS__)
#define HG_WARN(...)          ::Hog::Log::GetClientLogger()->warn(__VA_ARGS__)
#define HG_ERROR(...)         ::Hog::Log::GetClientLogger()->error(__VA_ARGS__)
#define HG_CRITICAL(...)      ::Hog::Log::GetClientLogger()->critical(__VA_ARGS__)
