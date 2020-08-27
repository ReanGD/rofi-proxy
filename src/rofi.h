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
    bool SetPrompt(const std::string& text);

private:
    std::shared_ptr<Logger> m_logger;
    Mode* m_proxyMode = nullptr;
    Mode* m_combiMode = nullptr;
};
