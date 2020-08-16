#include "proxy.h"

#include <string>
#include <cstdlib>
#include <filesystem>


namespace {

static std::filesystem::path FindLogPath() {
    std::filesystem::path XDGDataHome;
    if(const char* envValue = std::getenv("XDG_DATA_HOME"); envValue != nullptr) {
        XDGDataHome = envValue;
    } else if(const char* envValue = std::getenv("HOME"); envValue != nullptr) {
        XDGDataHome = envValue;
        XDGDataHome /= ".local";
        XDGDataHome /= "share";
    } else {
        throw std::runtime_error("not found env varaibles: XDG_DATA_HOME or HOME");
    }

    return XDGDataHome / "rofi" / "proxy.log";
}

}

Proxy::Proxy() {

}

void Proxy::Init() {
    m_log = fopen(FindLogPath().c_str(), "w");
    if (m_log == nullptr) {
        throw std::runtime_error("could not open log file");
    }

    fputs("Init\n", m_log);
}

unsigned int Proxy::GetLinesCount() const {
    return 0;
}

Proxy::~Proxy() {
    if (m_log != nullptr) {
        fputs("Destroy\n", m_log);
        fclose(m_log);
        m_log = nullptr;
    }
}
