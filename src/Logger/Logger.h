#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <vector>

enum LogType {
    LOG_INFO,
    LOG_ERROR,
    LOG_WARNING
};

struct LogEntry {
    LogType type;
    std::string message;
};

class Logger {
    public:
        static std::vector<LogEntry> entries;

        static void Log(const std::string &message);
        static void Error(const std::string &message);
        static void Warn(const std::string &message);
};

#endif
