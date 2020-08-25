#include "logger.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "exception.h"


Logger::Logger() {
    GetBuffer(1023);
}

Logger::~Logger() {
    if (m_logFd >= 0) {
        close(m_logFd);
        m_logFd = -1;
    }

    if (m_buf != nullptr) {
        delete[] m_buf;
        m_buf = nullptr;
    }

    m_bufSize = 0;
}

void Logger::EnableFileLogging() {
    std::string XDGDataHome;
    if(const char* envValue = std::getenv("XDG_DATA_HOME"); envValue != nullptr) {
        XDGDataHome = envValue;
    } else if(const char* envValue = std::getenv("HOME"); envValue != nullptr) {
        XDGDataHome = envValue + std::string("/.local/share");
    } else {
        throw ProxyError("not found env varaibles: XDG_DATA_HOME or HOME");
    }

    auto logDir = XDGDataHome + "/rofi";
    struct stat st;
    if (stat(logDir.c_str(), &st) == -1) {
       if (mkdir(logDir.c_str(), 0755) != 0) {
           throw ProxyError("can't create directory '%s' for write log", logDir.c_str());
       }
    }

    auto logPath = logDir + "/proxy.log";
    m_logFd = open(logPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
    if (m_logFd < 0) {
        throw ProxyError("can't open file '%s' for write log", logPath.c_str());
    }
}

char* Logger::GetBuffer(size_t size) {
    if (size <= m_bufSize) {
        return m_buf;
    }

    if (m_buf != nullptr) {
        delete[] m_buf;
    }

    m_bufSize = ((size >> 7) + 1) << 7;
    m_buf = new char[m_bufSize];

    return m_buf;
}

void Logger::Write(bool isError, const void* buffer, size_t count) {
    if (m_logFd >= 0) {
        ssize_t writteCount = ::write(m_logFd, buffer, count);
        if (writteCount != static_cast<ssize_t>(count)) {
            throw ProxyError("can't write log to file");
        }
    }

    if (isError) {
        auto writteCount = ::write(STDERR_FILENO, buffer, count);
        if (writteCount != static_cast<ssize_t>(count)) {
            throw ProxyError("can't write log to stderr");
        }
    }
}

void Logger::Raise(const char* text) {
    throw ProxyError(text);
}
