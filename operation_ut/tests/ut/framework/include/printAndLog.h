#ifndef _PRINT_AND_LOG_H_
#define _PRINT_AND_LOG_H_

#include <string>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <mutex>
#include <sstream>
#include "c_shell.h"

/* <-- 日志信息等级 --> */
enum LogLevel {LOG_DEBUG, INFO, WARNING, ERROR, CRITICAL};

// 初始化Logger实例
void InitLogger(const std::string& fileName = "log.txt", LogLevel level = INFO);
// 释放Logger实例
void ReleaseLogger();

void LogMessage(LogLevel level, const std::string& message);
void LogInfo(const std::string& message);
void LogWarning(const std::string& message);
void LogError(const std::string& message);
void LogCritical(const std::string& message);
void LogMemAddress(void* mem_addr, const std::string& fun_name);
void LogDebug(const std::string& message);



#endif
