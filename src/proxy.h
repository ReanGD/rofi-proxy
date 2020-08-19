#pragma once

#include <string>
#include <vector>

#include "process.h"


struct rofi_int_matcher_t;
class Proxy : public ProcessHandler {
    enum class State {
        Running,
        ErrorProcess,
        DestroyProcess,
        ChildFinished,
    };
public:
    Proxy();

public:
    void Init();
    void Destroy();

    size_t GetLinesCount() const;
    const char* GetLine(size_t index) const;
    bool TokenMatch(rofi_int_matcher_t** tokens, size_t index) const;
    const char* PreprocessInput(const char *text);

public:
    void OnReadLine(const char* text) override;
    void OnReadLineError(const char* text) override;
    void OnProcessExit(int pid, bool normally) override;

private:
    void Clear();

private:
    State m_state = State::Running;
    std::vector<std::string> m_lines;
    std::shared_ptr<Logger> m_logger;
    std::shared_ptr<Process> m_process;
};
