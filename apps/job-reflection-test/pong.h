#pragma once

#include <iostream>
#include <string>
#include <job_object.h>
#include "signal.h"

class Pong : public job::core::Object
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