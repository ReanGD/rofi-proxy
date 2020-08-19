#pragma once

#include <utility>


template <typename T> class Defer final {
public:
    Defer() = delete;
    Defer(Defer&) = delete;
    Defer(Defer&&) noexcept = delete;
    Defer& operator=(Defer&) = delete;
    Defer& operator=(Defer&&) noexcept = delete;

    inline Defer(T&& f) noexcept : m_callback(std::forward<T>(f)) {}
    ~Defer() {
        m_callback();
    }

private:
  T m_callback;
};
