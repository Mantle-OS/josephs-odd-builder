#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <gguf.h>

#include "job_ggml_tensor.h"
#include "job_gguf_kv.h"
#include "job_gguf_tensor_info.h"
#include "jobggml_export.h"

namespace job::ggml {

class JOBGGML_EXPORT JobGgufContext
{
public:
    using Ptr  = std::shared_ptr<JobGgufContext>;
    using WPtr = std::weak_ptr<JobGgufContext>;
    using UPtr = std::unique_ptr<JobGgufContext>;

    /*
     * Creates an empty native GGUF context.
     */
    JobGgufContext();

    /*
     * Takes ownership of an existing native GGUF context.
     *
     * The supplied context must have been created by one of the upstream
     * gguf_init_*() functions. It will be released through gguf_free().
     */
    explicit JobGgufContext(struct gguf_context *context);

    ~JobGgufContext();

    [[nodiscard]] static Ptr createShared()
    {
        return std::make_shared<JobGgufContext>();
    }

    [[nodiscard]] static Ptr createShared(struct gguf_context *context)
    {
        return std::make_shared<JobGgufContext>(context);
    }

    [[nodiscard]] static UPtr createUniq()
    {
        return std::make_unique<JobGgufContext>();
    }

    [[nodiscard]] static UPtr createUniq(struct gguf_context *context)
    {
        return std::make_unique<JobGgufContext>(context);
    }

    JobGgufContext(const JobGgufContext &) = delete;
    JobGgufContext &operator=(const JobGgufContext &) = delete;
    JobGgufContext(JobGgufContext &&) = delete;
    JobGgufContext &operator=(JobGgufContext &&) = delete;

    [[nodiscard]] bool isValid() const noexcept;

    // Native context
    [[nodiscard]] struct gguf_context *context() noexcept;
    [[nodiscard]] const struct gguf_context *context() const noexcept;

    /*
     * Releases the currently owned native context and leaves this object
     * invalid.
     */
    void reset() noexcept;

    /*
     * Replaces the currently owned native context.
     *
     * Ownership of context is transferred to this object.
     */
    void reset(struct gguf_context *context) noexcept;

    // ------------------------------------------------------------------------
    // General context metadata
    // ------------------------------------------------------------------------

    [[nodiscard]] std::uint32_t version() const noexcept;

    [[nodiscard]] std::size_t alignment() const noexcept;

    /*
     * Offset of the tensor-data section from the start of the GGUF file.
     */
    [[nodiscard]] std::size_t dataOffset() const noexcept;

    /*
     * Metadata size including the padding needed before tensor data.
     */
    [[nodiscard]] std::size_t metadataSize() const noexcept;

    /*
     * Copies serialized GGUF metadata into a caller-owned byte vector.
     */
    [[nodiscard]] std::vector<std::byte>
    metadata() const;

    // ------------------------------------------------------------------------
    // Key/value inspection
    // ------------------------------------------------------------------------

    [[nodiscard]] std::int64_t keyValueCount() const noexcept;
    [[nodiscard]] bool hasKey(const std::string &key) const noexcept;

    /*
     * Returns -1 when the key is not present.
     */
    [[nodiscard]] std::int64_t keyIndex(const std::string &key) const noexcept;
    [[nodiscard]] std::string key(std::int64_t index) const;

    [[nodiscard]] JobGgufType valueType(std::int64_t index) const;

    [[nodiscard]] enum gguf_type ggufValueType(std::int64_t index) const;

    /*
     * For an array, returns its element type.
     *
     * For a scalar value, this returns the same type as valueType().
     */
    [[nodiscard]] JobGgufType arrayElementType(std::int64_t index) const;

    [[nodiscard]] enum gguf_type ggufArrayElementType(std::int64_t index) const;

    [[nodiscard]] JobGgufKv::UPtr keyValue(std::int64_t index) const;

    [[nodiscard]] JobGgufKv::UPtr keyValue(const std::string &key) const;
    [[nodiscard]] std::vector<JobGgufKv::UPtr> keyValues() const;

    // ------------------------------------------------------------------------
    // Key/value mutation
    // ------------------------------------------------------------------------

    /*
     * Removes a key and returns its former index.
     *
     * Returns -1 when the key did not exist.
     */
    std::int64_t removeKey(const std::string &key);

    /*
     * Adds or replaces one key/value pair.
     *
     * Upstream places an added or replaced pair at the end of the native
     * key/value collection.
     */
    void setKeyValue(const JobGgufKv &keyValue);

    /*
     * Adds or replaces all key/value pairs from another context.
     */
    void setKeyValues(const JobGgufContext &source);

    // ------------------------------------------------------------------------
    // Tensor inspection
    // ------------------------------------------------------------------------
    [[nodiscard]] std::int64_t tensorCount() const noexcept;
    [[nodiscard]] bool hasTensor(const std::string &name) const noexcept;

    /*
     * Returns -1 when the tensor is not present.
     */
    [[nodiscard]] std::int64_t tensorIndex(const std::string &name) const noexcept;

    [[nodiscard]] std::string tensorName(std::int64_t index) const;

    [[nodiscard]] JobGgmlType tensorType(std::int64_t index) const;

    [[nodiscard]] enum ggml_type ggmlTensorType(std::int64_t index) const;

    [[nodiscard]] std::size_t tensorSize(std::int64_t index) const;

    [[nodiscard]] std::uint64_t tensorOffset(std::int64_t index) const;

    // [[nodiscard]] JobGgufTensorInfo::UPtr tensorInfo(std::int64_t index) const;
    // [[nodiscard]] JobGgufTensorInfo::UPtr tensorInfo(const std::string &name) const;
    // [[nodiscard]] std::vector<JobGgufTensorInfo::UPtr> tensorInfos() const;

    // ------------------------------------------------------------------------
    // Tensor mutation
    // ------------------------------------------------------------------------
    void addTensor(const JobGgmlTensor &tensor);

    void setTensorType(const std::string &name, JobGgmlType type);

    void setTensorData(const std::string &name, const void *data);
    void setTensorData(const std::string &name, const std::vector<std::byte> &data);

private:
    [[nodiscard]] bool validKeyIndex(std::int64_t index) const noexcept;
    [[nodiscard]] bool validTensorIndex(std::int64_t index) const noexcept;
    [[nodiscard]] JobGgufKv::UPtr buildKeyValue(std::int64_t index) const;
    // [[nodiscard]] JobGgufTensorInfo::UPtr buildTensorInfo(std::int64_t index) const;
    void setScalarKeyValue(const JobGgufKv &keyValue);
    void setArrayKeyValue(const JobGgufKv &keyValue);

    struct GgufContextDeleter
    {
        void operator()(struct gguf_context *context) const noexcept;
    };

    using NativeContextPtr = std::unique_ptr<struct gguf_context, GgufContextDeleter>;
    NativeContextPtr m_context;
};

} // namespace job::ggml