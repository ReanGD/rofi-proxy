#include "proxy.h"

#include <rofi/mode.h>
#include <rofi/helper.h>
#include <rofi/mode-private.h>

#include "rofi.h"
#include "logger.h"
#include "exception.h"

extern "C" {
extern void rofi_view_reload(void);

extern RofiViewState* rofi_view_get_active(void);
extern Mode* rofi_view_get_mode(RofiViewState *state);
extern void rofi_view_switch_mode(RofiViewState *state, Mode *mode);
extern void rofi_view_clear_input(RofiViewState *state);
extern void rofi_view_handle_text(RofiViewState *state, char *text);
}

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

    m_proxyMode = proxyMode;
    m_rofi->SetProxyMode(m_proxyMode);
    g_idle_add(OnPostInitHandler, this);

    m_logger->Debug("Init plugin finished");
}

void Proxy::OnPostInit() {
    m_logger->Debug("PostInit plugin start");

    Mode* mode = GetActiveRofiMode();
    if (mode == nullptr) {
        return;
    }

    if (std::string(mode->name) == "combi") {
        m_logger->Debug("Proxy runs under combi plugin");
        m_combiMode = mode;
        m_combiOriginPreprocessInput = m_combiMode->_preprocess_input;
        m_combiMode->_preprocess_input = m_proxyMode->_preprocess_input;
        m_rofi->SetCombiMode(m_combiMode);
    } else {
        m_combiMode = nullptr;
    }

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
    size_t result = m_lastRequest.lines.size();
    m_logger->Debug("GetLinesCount = %zu", result);
    return result;
}

const char* Proxy::GetLine(size_t index, int* state) {
    // m_logger->Debug("GetLine(%zu)", index);
    if (index >= m_lastRequest.lines.size()) {
        return nullptr;
    }

    if (index == 0) {
        const char* text = m_rofi->GetUserInput();
        if ((text != nullptr) && (*text == '\0')) {
            OnInput(nullptr, text);
        }
    }

    if (m_lastRequest.lines[index].markup) {
        *state |= MARKUP;
    }
    return m_lastRequest.lines[index].text.c_str();
}

const char* Proxy::GetHelpMessage() const {
    if (m_lastRequest.help.empty()) {
        m_logger->Debug("GetHelpMessage = \"\"");
        return nullptr;
    }

    m_logger->Debug("GetHelpMessage = \"%s\"", m_lastRequest.help.c_str());
    return m_lastRequest.help.c_str();
}

bool Proxy::OnCancel() {
    if (m_lastRequest.exitByCancel) {
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
    if (index >= m_lastRequest.lines.size()) {
        return;
    }

    try {
        std::string msg = m_protocol->CreateMessageSelectLine(m_lastRequest.lines[index]);
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
        if (index < m_lastRequest.lines.size()) {
            line = m_lastRequest.lines[index];
        }

        std::string msg = m_protocol->CreateMessageKeyPress(line, keyName.c_str());
        m_process->Write(msg.c_str());
        m_logger->Debug("Send message with name \"key_press\" to child process: %s", msg.c_str());
    } catch(const std::exception& e) {
        OnSendRequestError(e.what());
    }
}

const char* Proxy::OnInput(Mode* sw, const char* text) {
    m_logger->Debug("OnInput(\"%s\")", text);
    if (m_input == text) {
        return text;
    }

    try {
        m_input = text;
        std::string msg = m_protocol->CreateMessageInput(text);
        m_process->Write(msg.c_str());
        m_logger->Debug("Send message with name \"input\" to child process: %s", msg.c_str());
    } catch(const std::exception& e) {
        OnSendRequestError(e.what());
    }

    if ((text != nullptr) && (sw == m_combiMode) && (m_combiOriginPreprocessInput != nullptr)) {
        return m_combiOriginPreprocessInput(sw, text);
    }

    return text;
}

bool Proxy::OnTokenMatch(rofi_int_matcher_t** tokens, size_t index) const {
    // m_logger->Debug("OnTokenMatch(%zu)", index);
    if (index >= m_lastRequest.lines.size()) {
        return false;
    }

    if (!m_lastRequest.lines[index].filtering) {
        return true;
    }

    return (helper_token_match(tokens, m_lastRequest.lines[index].text.c_str()) == TRUE);
}

void Proxy::OnReadLine(const char* text) {
    m_logger->Debug("Get request from child process: %s", text);

    try {
        m_lastRequest = m_protocol->ParseRequest(text);
        RofiViewState* viewState = GetRofiViewState();
        if (viewState == nullptr) {
            return;
        }

        if (m_lastRequest.updateInput && (m_input != m_lastRequest.input)) {
            m_input = m_lastRequest.input;
            rofi_view_clear_input(viewState);
            char* input = g_strdup(m_input.c_str());
            rofi_view_handle_text(viewState, input);
            g_free(input);
        }

        if (m_lastRequest.updateOverlay) {
            m_rofi->SetOverlay(m_lastRequest.overlay);
        }

        Mode* newMode = nullptr;
        Mode* curMode = GetActiveRofiMode(viewState);
        if (curMode == nullptr) {
            return;
        }

        bool needSwitchModeByPrompt = (m_lastRequest.updatePrompt && m_rofi->SetPrompt(m_lastRequest.prompt));
        if (needSwitchModeByPrompt) {
            newMode = curMode;
        }

        bool needSwitchModeByCombiMode = false;
        if (m_combiMode != nullptr) {
            if (curMode == m_combiMode) {
                needSwitchModeByCombiMode = m_lastRequest.hideCombiLines;
            } else if (curMode == m_proxyMode) {
                needSwitchModeByCombiMode = !m_lastRequest.hideCombiLines;
            }

            if (needSwitchModeByCombiMode) {
                newMode = m_lastRequest.hideCombiLines ? m_proxyMode : m_combiMode;
            }
        }

        if (needSwitchModeByPrompt || needSwitchModeByCombiMode) {
            rofi_view_switch_mode(viewState, newMode);
        } else {
            rofi_view_reload();
        }
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

RofiViewState* Proxy::GetRofiViewState() {
    try {
        RofiViewState* viewState = rofi_view_get_active();
        if (viewState == nullptr) {
            throw ProxyError("rofi view state is null");
        }

        return viewState;
    } catch(const std::exception& e) {
        m_logger->Error("Error while getting rofi view state: %s", e.what());
        m_state = State::ErrorProcess;
        m_process->Kill();
        return nullptr;
    }
}

Mode* Proxy::GetActiveRofiMode(RofiViewState* viewState) {
    try {
        if (viewState == nullptr) {
            viewState = rofi_view_get_active();
        }
        if (viewState == nullptr) {
            throw ProxyError("rofi view state is null");
        }

        Mode* mode = rofi_view_get_mode(viewState);
        if (mode == nullptr) {
            throw ProxyError("rofi view mode is null");
        }

        return mode;
    } catch(const std::exception& e) {
        m_logger->Error("Error while getting active rofi mode: %s", e.what());
        m_state = State::ErrorProcess;
        m_process->Kill();
        return nullptr;
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
