#include "proxy.h"

#include <rofi/helper.h>

#include "logger.h"
#include "exception.h"


Proxy::Proxy()
    : m_logger(std::make_shared<Logger>()) {

}

void Proxy::Init() {
    char* logCmd = nullptr;
    if (find_arg_str("-proxy-log", &logCmd) == TRUE) {
        m_logger->EnableFileLogging();
    }
    m_logger->Debug("Init plugin start");

    m_lines.push_back("aaa1");
    m_lines.push_back("aaa2");
    m_lines.push_back("aaa3");
    m_process = std::make_shared<Process>(this, m_logger);

    char* command = nullptr;
    if (find_arg_str("-proxy-cmd", &command) == TRUE) {
        try {
            m_process->Start(command);
        } catch(const std::exception& e) {
            m_logger->Error("Unable to start child process %s", command);
            throw ProxyError("Unable to start child process %s", command);
        }
    } else {
        m_process->Start(nullptr);
    }

    m_logger->Debug("Init plugin finished");
}

void Proxy::Destroy() {
    m_logger->Debug("Destroy plugin start");
    m_state = State::DestroyProcess;
    if (m_process) {
        m_process->Kill();
    }
    m_logger->Debug("Destroy plugin finished");
    m_logger.reset();
}

size_t Proxy::GetLinesCount() const {
    size_t result = m_lines.size();
    m_logger->Debug("GetLinesCount = %zu", result);
    return result;
}

const char* Proxy::GetLine(size_t index) const {
    m_logger->Debug("GetLine(%zu)", index);
    if (index >= m_lines.size()) {
        return nullptr;
    }

    return m_lines[index].c_str();
}

bool Proxy::TokenMatch(rofi_int_matcher **tokens, size_t index) const {
    m_logger->Debug("TokenMatch(%zu)", index);
    if (index >= m_lines.size()) {
        return false;
    }

    return helper_token_match(tokens, m_lines[index].c_str()) == TRUE;
}

void Proxy::OnReadLine(const char* text) {
    m_logger->Debug("OnReadLine(%s)", text);
}

void Proxy::OnReadLineError(const char* text) {
    m_logger->Error("Error while reading from stdout child process: %s", text);
    if (m_state == State::Running) {
        m_state = State::ErrorProcess;
        m_process->Kill();
    }
}

void Proxy::OnProcessExit(int pid, bool normally) {
    if (m_state == State::Running) {
        m_logger->Error("Child process %" G_PID_FORMAT " exited unexpectedly, %s", pid, normally ? "normally" : "abnormally");
        Clear();
        exit(1);
    }

    if (m_state == State::ErrorProcess) {
        m_logger->Debug("Child process %" G_PID_FORMAT " exited %s", pid, normally ? "normally" : "abnormally");
        Clear();
        exit(1);
    }

    if (m_state == State::DestroyProcess) {
        m_logger->Debug("Child process %" G_PID_FORMAT " exited %s", pid, normally ? "normally" : "abnormally");
        m_process.reset();
    }
}

void Proxy::Clear() {
    m_process.reset();
    m_logger.reset();
}
