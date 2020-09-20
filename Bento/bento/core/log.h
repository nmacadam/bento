#pragma once

#include <memory>

#include <spdlog/spdlog.h>
#include "core.h"

namespace bento
{
	class log
	{
	public:
		static void initialize();

		template<typename FormatString, typename... Args> static void trace(const FormatString& fmt, const Args&... args) { GetLogger()->trace(fmt, args...); }
		template<typename T> static void trace(const T& msg) { GetLogger()->trace(msg); }

		template<typename FormatString, typename... Args> static void info(const FormatString& fmt, const Args&... args) { GetLogger()->info(fmt, args...); }
		static void info(const char* msg) { GetLogger()->info(msg); }

		template<typename FormatString, typename... Args> static void warn(const FormatString& fmt, const Args&... args) { GetLogger()->warn(fmt, args...); }
		template<typename T> static void warn(const T& msg) { GetLogger()->warn(msg); }

		template<typename FormatString, typename... Args> static void error(const FormatString& fmt, const Args&... args) { GetLogger()->error(fmt, args...); }
		template<typename T> static void error(const T& msg) { GetLogger()->error(msg); }

		//inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		//inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

		inline static std::shared_ptr<spdlog::logger>& GetLogger()
		{
			return s_CoreLogger;
		}

	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		//static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};

}
