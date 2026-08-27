#pragma once

#include <iostream>
#include <string>
#include "obj.h"
#include "signal.h"

class Ping : public Object
{
public:
    Ping() = default;
    ~Ping() = default;

    std::string name{"PingObject"};
    Signal<int> pingChanged;

    void emit(int value) {
        pingChanged.emit(value);
    }

    void handlePong(int value) {
        std::cout << "Ping::handlePong(" << value << ")\n";
    }
};

static_assert(ObjectType<Ping>);