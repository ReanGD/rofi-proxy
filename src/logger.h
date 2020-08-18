#pragma once

#include <cstdio>
#include <utility>


class Logger {
public:
    Logger() = delete;
    Logger(bool enableFileLogging);
    ~Logger();

public:
    template<typename ... Args> void debug(const char* format, Args&&... args) {
        writeWithFormat(false, format, std::forward<Args>(args)...);
    }

    template<typename ... Args> void error(const char* format, Args&&... args) {
        writeWithFormat(true, format, std::forward<Args>(args)...);
    }

private:
    char* getBuffer(size_t size);
    void write(bool isError, const void* buffer, size_t count);
    void raise(const char* text);

    template<typename ... Args>
    void writeWithFormat(bool isError, const char* format, Args&&... args) {
        if ((!isError) && (m_logFd < 0)) {
            return;
        }
        int cnt = snprintf(nullptr, 0, format, std::forward<Args>(args)...);
        if (cnt <= 0) {
            raise("error during log formatting");
        }
        size_t size = static_cast<size_t>(cnt) + 1; // extra space for '\0'

        char* buffer = getBuffer(size);
        snprintf(buffer, size, format, std::forward<Args>(args)...);
        buffer[cnt] = '\n'; // repce '\0' to '\n'
        write(isError, reinterpret_cast<const void*>(buffer), size);
    }

private:
    int m_logFd = -1;
    char* m_buf = nullptr;
    size_t m_bufSize = 0;
};
