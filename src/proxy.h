#pragma once

#include <string>
#include <vector>

#include "process.h"
#include "protocol.h"


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
    ~Proxy();

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
    std::string m_input;
    std::vector<Line> m_lines;
    std::shared_ptr<Logger> m_logger;
    std::unique_ptr<Process> m_process;
    std::unique_ptr<Protocol> m_protocol;
};
