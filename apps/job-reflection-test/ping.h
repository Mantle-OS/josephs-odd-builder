#pragma once

#include <iostream>

#include "obj.h"
#include "signal.h"

class Ping : public Object
{

public:
    Ping() = default;
    ~Ping() override = default;

    Signal<int> pingChanged;
    void emit(int value)
    {
        pingChanged.emit(value);
    }

    void handlePong(int value)
    {
        std::cout << "Ping::handlePong(" << value << ")\n";
    }
};