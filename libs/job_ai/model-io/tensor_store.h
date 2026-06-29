#pragma once

#include <gguf.h>

#include <memory>
#include <unordered_map>
class TensorStore {
public:
    using Ptr = std::shared_ptr<TensorStore>;
    // load, get, has, keys, etc.
private:
    gguf_context_ptr                                m_ggufCtx;
    std::unordered_map<std::string, ggml_tensor*>   m_tensors;
    std::string                                     m_architecture;
};