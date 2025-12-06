#pragma once

#include <memory>

#include "adapter_types.h"
#include "iadapter.h"

#include "dense_adapter.h"

namespace job::ai::adapters {

inline std::shared_ptr<I> makeThreadCtx(AdapterType t) {
    switch (t) {
    }


    return nullptr;
}
}
