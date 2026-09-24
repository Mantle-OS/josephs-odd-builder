#pragma once

#include <cstdint>

namespace job::render {

enum class JobColorChannel : std::uint8_t
{
    None,
    Red,
    Green,
    Blue,
    Alpha
};

}
