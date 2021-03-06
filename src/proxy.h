#pragma once

#include <string>
#include <vector>

#include "process.h"
#include "protocol.h"


class Rofi;
struct rofi_int_matcher_t;
typedef struct rofi_mode Mode;
typedef struct _cairo_surface cairo_surface_t;
class Proxy : public ProcessHandler {
    enum class State {
        Starting,
        Running,
        ErrorProcess,
        DestroyProcess,
        ChildFinished,
    };
public:
    Proxy();
    ~Proxy();

public:
    void Init(Mode* proxyMode);
    void OnPostInit();
    void Destroy();

    size_t GetLinesCount() const;
    const char* GetLine(size_t index, int* state);
    const char* GetHelpMessage() const;
    cairo_surface_t* GetIcon(size_t index, int height);

    bool OnCancel();
    void OnSelectLine(size_t index);
    void OnDeleteLine(size_t index);
    void OnSelectCustomInput(const char* text);
    void OnCustomKey(size_t index, int key);
    const char* OnInput(Mode* sw, const char* text);
    bool OnLineMatch(rofi_int_matcher_t** tokens, size_t index) const;

public:
    void OnReadLine(const char* text) override;
    void OnReadLineError(const char* text) override;
    void OnProcessExit(int pid, bool normally) override;

private:
    void SendMessage(const char* messageName, const std::string& messageText);
    void Clear();

private:
    std::string m_help;
    bool m_exitByCancel = true;
    std::vector<Line> m_lines;

    State m_state = State::Starting;
    std::shared_ptr<Logger> m_logger;
    std::unique_ptr<Rofi> m_rofi;
    std::unique_ptr<Process> m_process;
    std::unique_ptr<Protocol> m_protocol;
};
