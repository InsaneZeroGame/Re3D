#include "logger.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <chrono>


void InitLog()
{
	try
	{
		gLogger = spdlog::basic_logger_mt("engine_logger", "logs/re3d_log.txt",true);
		spdlog::flush_every(std::chrono::seconds(3));
		gLogger->info("Re3D Engine Startup");
		//gLogger = spdlog::stdout_color_mt("console");
		//auto err_logger = spdlog::stderr_color_mt("stderr");
		//spdlog::get("console")->info("loggers can be retrieved from a global registry using the spdlog::get(logger_name)");

	}
	catch (const spdlog::spdlog_ex& ex)
	{
		std::cout << "Log init failed: " << ex.what() << std::endl;
	}
}
