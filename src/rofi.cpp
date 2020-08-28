#include "rofi.h"

#include <rofi/mode.h>
#include <rofi/mode-private.h>

#include "logger.h"


extern "C" {
typedef struct RofiViewState RofiViewState;

extern RofiViewState* rofi_view_get_active(void);
extern const char* rofi_view_get_user_input(const RofiViewState *state);
extern void rofi_view_set_overlay(RofiViewState *state, const char *text);
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

void Rofi::SetCombiMode(Mode* mode) {
    m_combiMode = mode;
}

bool Rofi::SetPrompt(const std::string& text) {
    bool changed = false;
    if (m_proxyMode->display_name != text) {
        changed = true;
        if (m_proxyMode->display_name != nullptr) {
            g_free(m_proxyMode->display_name);
        }
        m_proxyMode->display_name = g_strdup(text.c_str());
    }

    if ((m_combiMode != nullptr) && ((m_combiMode->display_name == nullptr) || (m_combiMode->display_name != text))) {
        changed = true;
        if (m_combiMode->display_name != nullptr) {
            g_free(m_combiMode->display_name);
        }
        m_combiMode->display_name = g_strdup(text.c_str());
    }

    return changed;
}

void Rofi::SetOverlay(const std::string& text) {
    if (m_overlay == text) {
        return;
    }

    if (RofiViewState* viewState = rofi_view_get_active(); viewState != nullptr) {
        m_overlay = text;
        rofi_view_set_overlay(viewState, m_overlay.c_str());
    } else {
        m_logger->Debug("can't set overlay, rofi view is null");
    }
}

const char* Rofi::GetUserInput() noexcept {
    if (RofiViewState* viewState = rofi_view_get_active(); viewState != nullptr) {
        return rofi_view_get_user_input(viewState);
    }

    return nullptr;
}
