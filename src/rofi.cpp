#include "rofi.h"

extern "C" {
#include <rofi/mode.h>
#include <rofi/mode-private.h>
#include <rofi/rofi-icon-fetcher.h>
}

#include "logger.h"
#include "exception.h"


extern "C" {
extern RofiViewState* rofi_view_get_active(void);

extern Mode* rofi_view_get_mode(RofiViewState *state);
extern void rofi_view_reload(void);
extern void rofi_view_switch_mode(RofiViewState *state, Mode *mode);

extern void rofi_view_set_overlay(RofiViewState *state, const char *text);

extern const char* rofi_view_get_user_input(const RofiViewState *state);
extern void rofi_view_clear_input(RofiViewState *state);
extern void rofi_view_handle_text(RofiViewState *state, char *text);
}

namespace {

static void CopyMode(const Mode* src, Mode* dst) {
    dst->_get_num_entries = src->_get_num_entries;
    dst->_result = src->_result;
    dst->_token_match = src->_token_match;
    dst->_get_display_value = src->_get_display_value;
    dst->_get_icon = src->_get_icon;
    dst->_get_completion = src->_get_completion;
    // dst->_preprocess_input = src->_preprocess_input;
    dst->_get_message = src->_get_message;
}

}

Rofi::Rofi(const std::shared_ptr<Logger>& logger)
    : m_logger(logger) {

}

Rofi::~Rofi() {
    if (m_combiMode != nullptr) {
        CopyMode(m_combiModeOrigin, m_combiMode);
        m_combiMode->_preprocess_input = m_combiOriginPreprocessInput;
    }
    if (m_combiModeOrigin != nullptr) {
        delete m_combiModeOrigin;
        m_combiModeOrigin = nullptr;
    }
    m_logger.reset();
}

void Rofi::SetProxyMode(Mode* mode) {
    m_proxyMode = mode;
}

void Rofi::OnPostInit() {
    RofiViewState* viewState = rofi_view_get_active();
    if (viewState == nullptr) {
        throw ProxyError("rofi view state is null");
    }

    Mode* currentMode = rofi_view_get_mode(viewState);
    if (currentMode == nullptr) {
        throw ProxyError("rofi view mode is null");
    }

    if (std::string(currentMode->name) == "combi") {
        m_logger->Debug("Proxy runs under combi plugin");
        m_combiMode = currentMode;
        m_combiOriginPreprocessInput = m_combiMode->_preprocess_input;
        m_combiMode->_preprocess_input = m_proxyMode->_preprocess_input;

        m_combiModeOrigin = new Mode();
        CopyMode(m_combiMode, m_combiModeOrigin);
    }
}

const char* Rofi::GetActualUserInput() noexcept {
    if (RofiViewState* viewState = rofi_view_get_active(); viewState != nullptr) {
        return rofi_view_get_user_input(viewState);
    }

    return nullptr;
}

const char* Rofi::CallOriginPreprocessInput(Mode* sw, const char* text) {
    if ((text != nullptr) && (sw == m_combiMode) && (m_combiOriginPreprocessInput != nullptr)) {
        return m_combiOriginPreprocessInput(sw, text);
    }

    return text;
}

cairo_surface_t* Rofi::GetIcon(uint32_t& uid, const std::string& name, int size) {
    if (uid == 0) {
        uid = rofi_icon_fetcher_query(name.c_str(), size);
    }
    return rofi_icon_fetcher_get(uid);
}

void Rofi::StartUpdate() {
    m_reloadMode = false;
    m_viewState = rofi_view_get_active();
    if (m_viewState == nullptr) {
        throw ProxyError("rofi view state is null");
    }
    m_currentMode = rofi_view_get_mode(m_viewState);
    if (m_currentMode == nullptr) {
        throw ProxyError("rofi view mode is null");
    }
}

void Rofi::ApplyUpdate() {
    if (m_reloadMode) {
        rofi_view_switch_mode(m_viewState, m_currentMode);
    } else {
        rofi_view_reload();
    }
}

void Rofi::UpdatePrompt(const std::string& text) {
    if (m_proxyMode->display_name != text) {
        m_reloadMode = true;
        if (m_proxyMode->display_name != nullptr) {
            g_free(m_proxyMode->display_name);
        }
        m_proxyMode->display_name = g_strdup(text.c_str());
    }

    if ((m_combiMode != nullptr) && ((m_combiMode->display_name == nullptr) || (m_combiMode->display_name != text))) {
        m_reloadMode = true;
        if (m_combiMode->display_name != nullptr) {
            g_free(m_combiMode->display_name);
        }
        m_combiMode->display_name = g_strdup(text.c_str());
    }
}

void Rofi::UpdateOverlay(const std::string& text) {
    if (m_overlay == text) {
        return;
    }

    m_overlay = text;
    rofi_view_set_overlay(m_viewState, m_overlay.c_str());
}

void Rofi::UpdateHideCombiLines(bool value) {
    if (m_combiMode == nullptr) {
        return;
    }

    m_reloadMode = true;
    if (value) {
        CopyMode(m_proxyMode, m_combiMode);
    } else {
        CopyMode(m_combiModeOrigin, m_combiMode);
    }
}

void Rofi::UpdateUserInput(const std::string& text) {
    if (m_input == text) {
        return;
    }

    m_input = text;
    rofi_view_clear_input(m_viewState);
    char* input = g_strdup(m_input.c_str());
    rofi_view_handle_text(m_viewState, input);
    g_free(input);
}
