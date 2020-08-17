#pragma once

#include <string>
#include <memory>


template<typename ... Args>
std::string format(const std::string& format, Args... args) {
    int cnt = snprintf(nullptr, 0, format.c_str(), args...);
    if (cnt <= 0) {
        throw std::runtime_error("error during formatting");
    }
    size_t size = static_cast<size_t>(cnt) + 1; // Extra space for '\0'

    std::unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, format.c_str(), args...);

    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}
