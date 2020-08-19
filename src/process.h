#pragma once

#include <memory>


class ProcessHandler {
public:
    ProcessHandler() = default;
    virtual ~ProcessHandler() = default;

public:
    virtual void OnReadLine(const char* text) = 0;
    virtual void OnReadLineError(const char* text) = 0;
    virtual void OnProcessExit(int pid, bool normally) = 0;
};

class Logger;
struct _GIOChannel;
typedef struct _GIOChannel GIOChannel;

class Process {
public:
    Process() = delete;
    Process(ProcessHandler* handler, const std::shared_ptr<Logger>& logger);
    ~Process();

    void Start(const char* command);
    void Write(const char* text);
    void Kill();

private:
    void StartImpl(const char* command);
    void Spawn(char **argv);
    void SetNonBlockFlag(int fd);

private:
    int m_pid = -1;
    int m_readFd;
    int m_writeFd;
    unsigned int m_readChWatcher = 0;
    GIOChannel* m_readCh = nullptr;
    GIOChannel* m_writeCh = nullptr;
    ProcessHandler* m_handler = nullptr;
    std::shared_ptr<Logger> m_logger;
};
