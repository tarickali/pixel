#include "Logger.h"

#include <iostream>
#include <string>
#include <chrono>
#include <ctime>

std::vector<LogEntry> Logger::entries;

static std::string CurrentDateTimeToString() {
    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::string output(30, '\0');
    std::strftime(&output[0], output.size(), "%d-%b-%Y %H:%M:%S", std::localtime(&now));
    return output;
}

void Logger::Log(const std::string &message) {
    LogEntry entry;
    entry.type = LOG_INFO;
    entry.message = "LOG: [" + CurrentDateTimeToString() + "]: " + message;
    entries.push_back(entry);
    std::cout << "\033[32m" << entry.message << "\033[0m" << std::endl;
}

void Logger::Error(const std::string &message) {
    LogEntry entry;
    entry.type = LOG_ERROR;
    entry.message = "ERROR: [" + CurrentDateTimeToString() + "]: " + message;
    entries.push_back(entry);
    std::cerr << "\033[91m" << entry.message << "\033[0m" << std::endl;
}

void Logger::Warn(const std::string &message) {
    LogEntry entry;
    entry.type = LOG_WARNING;
    entry.message = "WARN: [" + CurrentDateTimeToString() + "]: " + message;
    entries.push_back(entry);
    std::cerr << "\033[33m" << entry.message << "\033[0m" << std::endl;
}
