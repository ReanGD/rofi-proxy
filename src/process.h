#pragma once


class ProxyBase;
struct _GIOChannel;
typedef struct _GIOChannel GIOChannel;
class Process {
public:
    Process() = delete;
    Process(ProxyBase* parent);
    ~Process();

    void Create(char **argv);
    void Destroy();

    void OnNewLine(const char* text);

private:
    void Spawn(char **argv);
    void SetNonBlockFlag(int fd);

private:
    int m_pid = 0;
    int m_readFd;
    int m_writeFd;
    unsigned int m_readChWatcher = 0;
    ProxyBase* m_parent = nullptr;
    GIOChannel* m_readCh = nullptr;
    GIOChannel* m_writeCh = nullptr;
};
