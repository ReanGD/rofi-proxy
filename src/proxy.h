#pragma once

#include <string>
#include <vector>

#include "process.h"
#include "protocol.h"


struct rofi_mode;
struct rofi_int_matcher_t;
typedef struct rofi_mode Mode;
typedef char* (*PreprocessInputCallback)(Mode *sw, const char *input);
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
    void Init(Mode* proxyMode);
    void OnPostInit();
    void Destroy();

    size_t GetLinesCount() const;
    const char* GetLine(size_t index, int* state);
    bool TokenMatch(rofi_int_matcher_t** tokens, size_t index) const;
    const char* PreprocessInput(Mode* sw, const char* text);
    void OnSelectLine(size_t index);

public:
    void OnReadLine(const char* text) override;
    void OnReadLineError(const char* text) override;
    void OnProcessExit(int pid, bool normally) override;

private:
    void OnSendRequestError(const char* err);
    void Clear();

private:
    State m_state = State::Running;
    std::string m_input;
    std::vector<Line> m_lines;
    std::shared_ptr<Logger> m_logger;
    std::unique_ptr<Process> m_process;
    std::unique_ptr<Protocol> m_protocol;

    Mode* m_proxyMode = nullptr;
    Mode* m_combiMode = nullptr;
    PreprocessInputCallback m_combiOriginPreprocessInput = nullptr;
};
