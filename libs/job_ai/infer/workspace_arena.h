#pragma once
#include <cstddef>
#include "workspace.h"

namespace job::ai::infer {
struct WorkspaceArena {
    Workspace           &ws;
    std::size_t         cursorBytes = 0;

    static constexpr std::size_t align_up(std::size_t x, std::size_t a)
    {
        return (x + (a - 1)) & ~(a - 1);
    }

    std::byte *allocBytes(std::size_t nBytes, std::size_t alignBytes = 64)
    {
        cursorBytes = align_up(cursorBytes, alignBytes);
        if (cursorBytes + nBytes > ws.size())
            return nullptr;

        auto* p = reinterpret_cast<std::byte*>(ws.raw()) + cursorBytes;
        cursorBytes += nBytes;
        return p;
    }

    float *allocFloats(std::size_t nFloats, std::size_t alignBytes = 64)
    {
        auto *p = allocBytes(nFloats * sizeof(float), alignBytes);
        return reinterpret_cast<float*>(p);
    }
};

}
