#include "rofi.h"

#include <rofi/mode.h>
#include <rofi/mode-private.h>

#include "logger.h"


extern "C" {
// extern void rofi_view_reload(void);

// extern RofiViewState* rofi_view_get_active(void);
// extern Mode* rofi_view_get_mode(RofiViewState *state);
// extern void rofi_view_switch_mode(RofiViewState *state, Mode *mode);
// extern void rofi_view_clear_input(RofiViewState *state);
// extern void rofi_view_handle_text(RofiViewState *state, char *text);
// extern const char* rofi_view_get_user_input(const RofiViewState *state);
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
