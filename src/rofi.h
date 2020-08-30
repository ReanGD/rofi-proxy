#pragma once

#include <memory>
#include <string>


typedef struct rofi_mode Mode;
typedef struct RofiViewState RofiViewState;
typedef struct _cairo_surface cairo_surface_t;
typedef char* (*PreprocessInputCallback)(Mode* sw, const char* input);
class Logger;
class Rofi {
public:
    Rofi() = delete;
    Rofi(const std::shared_ptr<Logger>& logger);
    ~Rofi();

public:
    void SetProxyMode(Mode* mode);
    void OnPostInit();

    std::string GetCachedUserInput() const { return m_input; }
    void SetCachedUserInput(const std::string& text) { m_input = text; }
    const char* GetActualUserInput() noexcept;
    const char* CallOriginPreprocessInput(Mode* sw, const char* text);

    cairo_surface_t* GetIcon(uint32_t& uid, const std::string& name, int size);

    void StartUpdate();
    void ApplyUpdate();

    void UpdatePrompt(const std::string& text);
    void UpdateOverlay(const std::string& text);
    void UpdateHideCombiLines(bool value);
    void UpdateUserInput(const std::string& text);

private:
    std::string m_input;
    std::string m_overlay;

    bool m_reloadMode = false;
    RofiViewState* m_viewState = nullptr;
    Mode* m_currentMode = nullptr;

    Mode* m_proxyMode = nullptr;
    Mode* m_combiMode = nullptr;
    Mode* m_combiModeOrigin = nullptr;
    PreprocessInputCallback m_combiOriginPreprocessInput = nullptr;
    std::shared_ptr<Logger> m_logger;
};
