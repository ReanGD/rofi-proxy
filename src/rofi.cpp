#include "rofi.h"

extern "C" {
#include <rofi/mode.h>
#include <rofi/mode-private.h>
#include <rofi/rofi-icon-fetcher.h>
}

#include "logger.h"


extern "C" {
typedef struct RofiViewState RofiViewState;
extern RofiViewState* rofi_view_get_active(void);

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

void Rofi::SetCombiMode(Mode* mode) {
    m_combiMode = mode;
}

cairo_surface_t* Rofi::GetIcon(uint32_t& uid, const std::string& name, int size) {
    if (uid == 0) {
        uid = rofi_icon_fetcher_query(name.c_str(), size);
    }
    return rofi_icon_fetcher_get(uid);
}

bool Rofi::UpdatePrompt(const std::string& text) {
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

void Rofi::UpdateOverlay(const std::string& text) {
    if (m_overlay == text) {
        return;
    }

    if (RofiViewState* viewState = rofi_view_get_active(); viewState != nullptr) {
        m_overlay = text;
        rofi_view_set_overlay(viewState, m_overlay.c_str());
    } else {
        m_logger->Debug("can't update overlay, rofi view is null");
    }
}

std::string Rofi::GetCachedUserInput() const {
    return m_input;
}

void Rofi::SetCachedUserInput(const std::string& text) {
    m_input = text;
}

const char* Rofi::GetActualUserInput() noexcept {
    if (RofiViewState* viewState = rofi_view_get_active(); viewState != nullptr) {
        return rofi_view_get_user_input(viewState);
    }

    return nullptr;
}

void Rofi::UpdateUserInput(const std::string& text) {
    if (m_input == text) {
        return;
    }

    if (RofiViewState* viewState = rofi_view_get_active(); viewState != nullptr) {
        m_input = text;
        rofi_view_clear_input(viewState);
        char* input = g_strdup(m_input.c_str());
        rofi_view_handle_text(viewState, input);
        g_free(input);
    } else {
        m_logger->Debug("can't update user input, rofi view is null");
    }
}
