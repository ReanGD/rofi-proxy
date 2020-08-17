#pragma once


struct _GIOChannel;
typedef struct _GIOChannel GIOChannel;
class Process {
public:
    Process();
    ~Process();

    void Create(char **argv = nullptr);
    void Destroy();

private:
    void Spawn(char **argv);
    void SetNonBlockFlag(int fd);

private:
    int m_pid = 0;
    int m_readFd = 0;
    int m_writeFd = 0;
    unsigned int m_readChWatcher = 0;
    GIOChannel* m_readCh = nullptr;
    GIOChannel* m_writeCh = nullptr;
};
