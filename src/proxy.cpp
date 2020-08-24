#include "proxy.h"

#include <rofi/helper.h>

#include "logger.h"
#include "exception.h"


extern "C" {
typedef struct RofiViewState RofiViewState;

extern void rofi_view_reload(void);
extern RofiViewState* rofi_view_get_active(void);
extern const char* rofi_view_get_user_input(const RofiViewState *state);
}

static const int NORMAL = 0;
static const int URGENT = 1;
static const int ACTIVE = 2;
static const int SELECTED = 4;
static const int MARKUP = 8;

Proxy::Proxy()
    : m_logger(std::make_shared<Logger>())
    , m_process(std::make_unique<Process>(this, m_logger))
    , m_protocol(std::make_unique<Protocol>()) {

}

Proxy::~Proxy() {

}

void Proxy::Init() {
    char* logCmd = nullptr;
    if (find_arg_str("-proxy-log", &logCmd) == TRUE) {
        m_logger->EnableFileLogging();
    }
    m_logger->Debug("Init plugin start");

    char* command = nullptr;
    if (find_arg_str("-proxy-cmd", &command) == TRUE) {
        m_process->Start(command);
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
        while (m_state != State::ChildFinished) {
            g_main_context_iteration(nullptr, TRUE);
        }
        m_process.reset();
    }
    m_protocol.reset();
    m_logger->Debug("Destroy plugin finished");
    m_logger.reset();
}

size_t Proxy::GetLinesCount() const {
    size_t result = m_lines.size();
    m_logger->Debug("GetLinesCount = %zu", result);
    return result;
}

const char* Proxy::GetLine(size_t index, int* state) {
    m_logger->Debug("GetLine(%zu)", index);
    if (index >= m_lines.size()) {
        return nullptr;
    }

    if (index == 0) {
        RofiViewState* viewState = rofi_view_get_active();
        if (viewState != nullptr) {
            const char* text = rofi_view_get_user_input(viewState);
            if ((text != nullptr) && (*text == '\0')) {
                PreprocessInput(text);
            }
        }
    }

    if (m_lines[index].markup) {
        *state |= MARKUP;
    }
    return m_lines[index].text.c_str();
}

bool Proxy::TokenMatch(rofi_int_matcher_t** tokens, size_t index) const {
    // m_logger->Debug("TokenMatch(%zu)", index);
    if (index >= m_lines.size()) {
        return false;
    }

    if (!m_lines[index].filtering) {
        return true;
    }

    return (helper_token_match(tokens, m_lines[index].text.c_str()) == TRUE);
}

const char* Proxy::PreprocessInput(const char *text) {
    m_logger->Debug("PreprocessInput(\"%s\")", text);
    if (m_input == text) {
        return text;
    }

    try {
        m_input = text;
        std::string request = m_protocol->CreateInputChangeRequest(text);
        m_process->Write(request.c_str());
        m_logger->Debug("Send input change request to child process: %s", request.c_str());
    } catch(const std::exception& e) {
        OnSendRequestError(e.what());
    }

    return text;
}

void Proxy::OnSelectLine(size_t index) {
    m_logger->Debug("OnSelectLine(%zu)", index);
    if (index >= m_lines.size()) {
        return;
    }

    try {
        std::string request = m_protocol->CreateOnSelectLineRequest(m_lines[index].id.c_str());
        m_process->Write(request.c_str());
        m_logger->Debug("Send on select line request to child process: %s", request.c_str());
    } catch(const std::exception& e) {
        OnSendRequestError(e.what());
    }
}

void Proxy::OnReadLine(const char* text) {
    m_logger->Debug("Get request from child process: %s", text);

    try {
        m_lines = m_protocol->ParseRequest(text);
        rofi_view_reload();
    } catch(const std::exception& e) {
        m_logger->Error("Error while parse child process request: %s", e.what());
        m_state = State::ErrorProcess;
        m_process->Kill();
    }
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
        if (normally) {
            m_logger->Debug("Child process %" G_PID_FORMAT " exited unexpectedly, normally", pid);
        } else {
            m_logger->Error("Child process %" G_PID_FORMAT " exited unexpectedly, abnormally", pid);
        }
        Clear();
        if (normally) {
            exit(0);
        }
        exit(1);
    }

    if (m_state == State::ErrorProcess) {
        m_logger->Debug("Child process %" G_PID_FORMAT " exited %s", pid, normally ? "normally" : "abnormally");
        Clear();
        exit(1);
    }

    if (m_state == State::DestroyProcess) {
        m_logger->Debug("Child process %" G_PID_FORMAT " exited %s", pid, normally ? "normally" : "abnormally");
        m_state = State::ChildFinished;
    }
}

void Proxy::OnSendRequestError(const char* err) {
    m_logger->Error("Error while send request to child process: %s", err);
    m_state = State::ErrorProcess;
    m_process->Kill();
}

void Proxy::Clear() {
    m_protocol.reset();
    m_process.reset();
    m_logger.reset();
}
