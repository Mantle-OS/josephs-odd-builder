#pragma once

#include <iostream>
#include "obj.h"
#include "signal.h"

class Pong : public Object
{
public:
    Pong() = default;
    ~Pong() override = default;

    Signal<int> pongChanged;

    void emit(int value)
    {
        pongChanged.emit(value);
    }

    void handlePing(int value)
    {
        std::cout << "Pong::handlePing(" << value << ")\n";
    }
};