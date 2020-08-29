#pragma once

#include <memory>
#include <string>


struct rofi_mode;
typedef struct rofi_mode Mode;
class Logger;
class Rofi {
public:
    Rofi() = delete;
    Rofi(const std::shared_ptr<Logger>& logger);
    ~Rofi();

public:
    void SetProxyMode(Mode* mode);
    void SetCombiMode(Mode* mode);

    // return true if prompt changed
    bool UpdatePrompt(const std::string& text);
    void UpdateOverlay(const std::string& text);

    std::string GetCachedUserInput() const;
    void SetCachedUserInput(const std::string& text);

    const char* GetActualUserInput() noexcept;
    void UpdateUserInput(const std::string& text);

private:
    std::string m_input;
    std::string m_overlay;

    Mode* m_proxyMode = nullptr;
    Mode* m_combiMode = nullptr;
    std::shared_ptr<Logger> m_logger;
};
