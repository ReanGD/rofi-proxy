#pragma once

class ProxyBase {
public:
    ProxyBase() = default;
    virtual ~ProxyBase() = default;

public:
    virtual void OnNewLine(const char* text);
};
