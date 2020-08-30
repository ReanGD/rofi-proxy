#include "proxy.h"

#include <rofi/helper.h>

#include "rofi.h"
#include "logger.h"
#include "exception.h"


static const int NORMAL = 0;
static const int URGENT = 1;
static const int ACTIVE = 2;
static const int SELECTED = 4;
static const int MARKUP = 8;

namespace {

static int OnPostInitHandler(void* ptr) {
    reinterpret_cast<Proxy*>(ptr)->OnPostInit();
    return FALSE;
}

}

Proxy::Proxy()
    : m_logger(std::make_shared<Logger>())
    , m_rofi(std::make_unique<Rofi>(m_logger))
    , m_process(std::make_unique<Process>(this, m_logger))
    , m_protocol(std::make_unique<Protocol>()) {

}

Proxy::~Proxy() {

}

void Proxy::Init(Mode* proxyMode) {
    char* logCmd = nullptr;
    if (find_arg_str("-proxy-log", &logCmd) == TRUE) {
        m_logger->EnableFileLogging();
    }
    m_logger->Debug("Init plugin start");

    m_rofi->SetProxyMode(proxyMode);
    g_idle_add(OnPostInitHandler, this);

    m_logger->Debug("Init plugin finished");
}

void Proxy::OnPostInit() {
    m_logger->Debug("PostInit plugin start");

    m_rofi->OnPostInit();

    m_state = State::Running;

    char* command = nullptr;
    if (find_arg_str("-proxy-cmd", &command) == TRUE) {
        m_process->Start(command);
    } else {
        m_process->Start(nullptr);
    }

    m_logger->Debug("PostInit plugin finished");
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
    m_rofi.reset();
    m_logger->Debug("Destroy plugin finished");
    m_logger.reset();
}

size_t Proxy::GetLinesCount() const {
    size_t result = m_lines.size();
    m_logger->Debug("GetLinesCount = %zu", result);
    return result;
}

const char* Proxy::GetLine(size_t index, int* state) {
    // m_logger->Debug("GetLine(%zu)", index);
    if (index >= m_lines.size()) {
        return nullptr;
    }

    if (index == 0) {
        const char* text = m_rofi->GetActualUserInput();
        if ((text != nullptr) && (*text == '\0')) {
            OnInput(nullptr, text);
        }
    }

    if (m_lines[index].markup) {
        *state |= MARKUP;
    }
    return m_lines[index].text.c_str();
}

const char* Proxy::GetHelpMessage() const {
    if (m_help.empty()) {
        m_logger->Debug("GetHelpMessage = \"\"");
        return nullptr;
    }

    m_logger->Debug("GetHelpMessage = \"%s\"", m_help.c_str());
    return m_help.c_str();
}

cairo_surface_t* Proxy::GetIcon(size_t index, int height) {
    // m_logger->Debug("GetIcon(%zu, %d)", index, height);
    if (index >= m_lines.size()) {
        return nullptr;
    }

    const std::string& name = m_lines[index].icon;
    if (name.empty()) {
        return nullptr;
    }

    return m_rofi->GetIcon(m_lines[index].iconUID, name, height);
}

bool Proxy::OnCancel() {
    if (m_exitByCancel) {
        m_logger->Debug("OnCancel = true (exit)");
        return true;
    }

    try {
        std::string msg = m_protocol->CreateMessageKeyPress(Line(), "cancel");
        m_process->Write(msg.c_str());
        m_logger->Debug("Send message with name \"key_press\" to child process: %s", msg.c_str());
    } catch(const std::exception& e) {
        OnSendRequestError(e.what());
    }

    m_logger->Debug("OnCancel = false (not exit)");
    return false;
}

void Proxy::OnSelectLine(size_t index) {
    m_logger->Debug("OnSelectLine(%zu)", index);
    if (index >= m_lines.size()) {
        return;
    }

    try {
        std::string msg = m_protocol->CreateMessageSelectLine(m_lines[index]);
        m_process->Write(msg.c_str());
        m_logger->Debug("Send message with name \"select_line\" to child process: %s", msg.c_str());
    } catch(const std::exception& e) {
        OnSendRequestError(e.what());
    }
}

void Proxy::OnCustomKey(size_t index, int key) {
    m_logger->Debug("OnCustomKey(line = %zu, key = %d)", index, key);

    try {
        auto keyName = "custom_" + std::to_string(key);
        Line line;
        if (index < m_lines.size()) {
            line = m_lines[index];
        }

        std::string msg = m_protocol->CreateMessageKeyPress(line, keyName.c_str());
        m_process->Write(msg.c_str());
        m_logger->Debug("Send message with name \"key_press\" to child process: %s", msg.c_str());
    } catch(const std::exception& e) {
        OnSendRequestError(e.what());
    }
}

const char* Proxy::OnInput(Mode* sw, const char* text) {
    if (m_rofi->GetCachedUserInput() == text) {
        return text;
    }
    m_logger->Debug("OnInput(\"%s\")", text);

    try {
        m_rofi->SetCachedUserInput(text);
        std::string msg = m_protocol->CreateMessageInput(text);
        m_process->Write(msg.c_str());
        m_logger->Debug("Send message with name \"input\" to child process: %s", msg.c_str());
    } catch(const std::exception& e) {
        OnSendRequestError(e.what());
    }

    return m_rofi->CallOriginPreprocessInput(sw, text);
}

bool Proxy::OnTokenMatch(rofi_int_matcher_t** tokens, size_t index) const {
    // m_logger->Debug("OnTokenMatch(%zu)", index);
    if (index >= m_lines.size()) {
        return false;
    }

    if (!m_lines[index].filtering) {
        return true;
    }

    return (helper_token_match(tokens, m_lines[index].text.c_str()) == TRUE);
}

void Proxy::OnReadLine(const char* text) {
    m_logger->Debug("Get request from child process: %s", text);

    try {
        auto request = m_protocol->ParseRequest(text);

        m_rofi->StartUpdate();

        if (request.updateLines) {
            m_lines = request.lines;
        }

        if (request.updateHelp) {
            m_help = request.help;
        }

        if (request.updateExitByCancel) {
            m_exitByCancel = request.exitByCancel;
        }

        if (request.updateInput) {
            m_rofi->UpdateUserInput(request.input);
        }

        if (request.updateOverlay) {
            m_rofi->UpdateOverlay(request.overlay);
        }

        if (request.updatePrompt) {
            m_rofi->UpdatePrompt(request.prompt);
        }

        if (request.updateHideCombiLines) {
            m_rofi->UpdateHideCombiLines(request.hideCombiLines);
        }

        m_rofi->ApplyUpdate();
    } catch(const std::exception& e) {
        m_logger->Error("Error error while applying state from child process request: %s", e.what());
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
    m_logger->Error("Error while send message to child process: %s", err);
    m_state = State::ErrorProcess;
    m_process->Kill();
}

void Proxy::Clear() {
    m_protocol.reset();
    m_process.reset();
    m_rofi.reset();
    m_logger.reset();
}
