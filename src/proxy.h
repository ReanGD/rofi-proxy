#pragma once

#include <cstdio>


class Proxy {
public:
    Proxy();
    ~Proxy();

public:
    void Init();
    unsigned int GetLinesCount() const;

private:
    FILE* m_log = nullptr;
};
