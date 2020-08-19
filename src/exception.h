#pragma once

#include <string>
#include <memory>
#include <stdexcept>


namespace detail {

template<typename ... Args>
std::string Format(const char* format, Args&&... args) {
    int cnt = snprintf(nullptr, 0, format, std::forward<Args>(args)...);
    if (cnt <= 0) {
        throw std::runtime_error("error during exception formatting");
    }
    size_t size = static_cast<size_t>(cnt) + 1; // extra space for '\0'

    std::unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, format, std::forward<Args>(args)...);

    return std::string(buf.get(), buf.get() + size - 1); // we don't want the '\0' inside
}

}

class ProxyError : public std::runtime_error {
public:
    explicit ProxyError(const std::string& text)
        : std::runtime_error(text) {}

    template<typename... Args> ProxyError(const char* format, Args&&... args)
        : std::runtime_error(detail::Format(format, std::forward<Args>(args)...)) {}
};
