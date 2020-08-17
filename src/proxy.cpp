#include "proxy.h"

#include <cstdlib>
#include <filesystem>
#include <rofi/helper.h>


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
    m_lines.push_back("aaa1");
    m_lines.push_back("aaa2");
    m_lines.push_back("aaa3");

    fputs("Init\n", m_log);
}

size_t Proxy::GetLinesCount() const {
    fputs("GetLinesCount\n", m_log);
    return m_lines.size();
}

const char* Proxy::GetLine(size_t index) const {
    std::string logMsg = "GetLine " + std::to_string(index) + "\n";
    fputs(logMsg.c_str(), m_log);
    if (index >= m_lines.size()) {
        return nullptr;
    }

    return m_lines[index].c_str();
}

bool Proxy::TokenMatch(rofi_int_matcher **tokens, size_t index) const {
    std::string logMsg = "TokenMatch " + std::to_string(index) + "\n";
    fputs(logMsg.c_str(), m_log);
    if (index >= m_lines.size()) {
        return false;
    }

    return helper_token_match(tokens, m_lines[index].c_str()) == TRUE;
}

Proxy::~Proxy() {
    if (m_log != nullptr) {
        fputs("Destroy\n", m_log);
        fclose(m_log);
        m_log = nullptr;
    }
}
