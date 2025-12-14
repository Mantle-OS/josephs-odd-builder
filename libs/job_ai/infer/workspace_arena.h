#pragma once
#include <cstddef>
#include "workspace.h"

namespace job::ai::infer {
struct WorkspaceArena {
    Workspace       &ws;
    std::size_t     cursorBytes = 0;

    static constexpr std::size_t align_up(std::size_t x, std::size_t a)
    {
        return (x + (a - 1)) & ~(a - 1);
    }


    std::byte *allocBytes(std::size_t nBytes, std::size_t alignBytes = 64)
    {
        cursorBytes = align_up(cursorBytes, alignBytes);
        if (cursorBytes + nBytes > ws.size())   // ws.size() is BYTES
            return nullptr;

        auto *p = reinterpret_cast<std::byte*>(ws.raw()) + cursorBytes;
        cursorBytes += nBytes;
        return p;
    }

    float *allocFloats(std::size_t nFloats, std::size_t alignBytes = 64)
    {
        return reinterpret_cast<float*>(allocBytes(nFloats * sizeof(float), alignBytes));
    }
};

}
