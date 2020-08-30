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

Rofi::Rofi(const std::shared_ptr<Logger>& logger)
    : m_logger(logger) {

}

Rofi::~Rofi() {
    m_logger.reset();
}

void Rofi::SetProxyMode(Mode* mode) {
    m_proxyMode = mode;
}

Mode* Rofi::ReadCombiMode() {
    RofiViewState* viewState = rofi_view_get_active();
    if (viewState == nullptr) {
        throw ProxyError("rofi view state is null");
    }

    Mode* currentMode = rofi_view_get_mode(viewState);
    if (currentMode == nullptr) {
        throw ProxyError("rofi view mode is null");
    }

    if (std::string(currentMode->name) == "combi") {
        m_combiMode = currentMode;
    } else {
        m_combiMode = nullptr;
    }

    return m_combiMode;
}

const char* Rofi::GetActualUserInput() noexcept {
    if (RofiViewState* viewState = rofi_view_get_active(); viewState != nullptr) {
        return rofi_view_get_user_input(viewState);
    }

    return nullptr;
}

cairo_surface_t* Rofi::GetIcon(uint32_t& uid, const std::string& name, int size) {
    if (uid == 0) {
        uid = rofi_icon_fetcher_query(name.c_str(), size);
    }
    return rofi_icon_fetcher_get(uid);
}

void Rofi::StartUpdate() {
    m_updateMode = false;
    m_updatePrompt = false;
    m_viewState = rofi_view_get_active();
    if (m_viewState == nullptr) {
        throw ProxyError("rofi view state is null");
    }
    m_currentMode = rofi_view_get_mode(m_viewState);
    if (m_currentMode == nullptr) {
        throw ProxyError("rofi view mode is null");
    }
    m_newMode = m_currentMode;
}

void Rofi::ApplyUpdate() {
    if (m_updateMode || m_updatePrompt) {
        rofi_view_switch_mode(m_viewState, m_newMode);
    } else {
        rofi_view_reload();
    }
}

void Rofi::UpdatePrompt(const std::string& text) {
    if (m_proxyMode->display_name != text) {
        m_updatePrompt = true;
        if (m_proxyMode->display_name != nullptr) {
            g_free(m_proxyMode->display_name);
        }
        m_proxyMode->display_name = g_strdup(text.c_str());
    }

    if ((m_combiMode != nullptr) && ((m_combiMode->display_name == nullptr) || (m_combiMode->display_name != text))) {
        m_updatePrompt = true;
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

    if (m_currentMode == m_combiMode) {
        m_updateMode = value;
    } else if (m_currentMode == m_proxyMode) {
        m_updateMode = !value;
    }

    if (m_updateMode) {
        m_newMode = value ? m_proxyMode : m_combiMode;
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
