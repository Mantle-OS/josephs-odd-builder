#pragma once

#include <iostream>
#include <string>
#include "obj.h"
#include "signal.h"

class Pong : public Object
{
public:
    Pong() = default;
    ~Pong() = default;

    std::string name{"PongObject"};
    Signal<int> pongChanged;

    void emit(int value) {
        pongChanged.emit(value);
    }

    void handlePing(int value) {
        std::cout << "Pong::handlePing(" << value << ")\n";
    }
};

static_assert(ObjectType<Pong>);