#pragma once

#include <string>
#include <vector>

#include "process.h"
#include "protocol.h"


class Rofi;
struct rofi_mode;
struct rofi_int_matcher_t;
typedef struct rofi_mode Mode;
typedef struct RofiViewState RofiViewState;
typedef char* (*PreprocessInputCallback)(Mode *sw, const char *input);
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

    bool OnCancel();
    void OnSelectLine(size_t index);
    void OnCustomKey(size_t index, int key);
    const char* OnInput(Mode* sw, const char* text);
    bool OnTokenMatch(rofi_int_matcher_t** tokens, size_t index) const;

public:
    void OnReadLine(const char* text) override;
    void OnReadLineError(const char* text) override;
    void OnProcessExit(int pid, bool normally) override;

private:
    RofiViewState* GetRofiViewState(bool skipException = false);
    Mode* GetActiveRofiMode(RofiViewState* viewState = nullptr);
    void OnSendRequestError(const char* err);
    void Clear();

private:
    // user state
    std::string m_input;
    UserRequest m_lastRequest;

    // modes state
    Mode* m_proxyMode = nullptr;
    Mode* m_combiMode = nullptr;
    PreprocessInputCallback m_combiOriginPreprocessInput = nullptr;

    // proxy state
    State m_state = State::Starting;
    std::shared_ptr<Logger> m_logger;
    std::unique_ptr<Rofi> m_rofi;
    std::unique_ptr<Process> m_process;
    std::unique_ptr<Protocol> m_protocol;
};
