#include <cstddef>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include <job_ggml_backend_buffer.h>
#include <job_ggml_backend_buffer_type.h>
#include <job_ggml_context.h>
#include <job_ggml_cpu.h>
#include <job_ggml_device_manager.h>
#include <job_ggml_enums.h>
#include <job_ggml_tensor.h>
#include <alloc/job_ggml_tensor_allocator.h>

using namespace job::ggml;

TEST_CASE("Tensor allocator can allocate a buffer sized for one I32 tensor",
          "[ggml][allocator][edge][alignment]")
{
    JobGgmlDeviceManager manager;

    REQUIRE(manager.isValid());
    JobGgmlCpu *cpu = manager.cpu();

    REQUIRE(cpu != nullptr);
    REQUIRE(cpu->bufferType() != nullptr);
    JobGgmlBackendBufferType *bufferType = cpu->bufferType();

    auto context = JobGgmlContext::createUniqMetadata(16);

    REQUIRE(context != nullptr);

    auto tensor = context->newTensor1d(JobGgmlType::I32, 70);

    REQUIRE(tensor != nullptr);

    const std::size_t reportedSize = bufferType->allocationSize(*tensor);
    const std::size_t allocatorSize = tensor->paddedByteCount();

    WARN("LOOK tensor byte count: " << tensor->byteCount());
    WARN("LOOK tensor padded byte count: " << tensor->paddedByteCount());
    WARN("LOOK reported allocation size: " << reportedSize);
    WARN("LOOK allocator allocation size: " << allocatorSize);
    WARN("LOOK buffer alignment: " << bufferType->alignment());

    auto uniqueBuffer = bufferType->allocateBuffer(allocatorSize);

    REQUIRE(uniqueBuffer != nullptr);

    JobGgmlBackendBuffer::Ptr buffer{
        std::move(uniqueBuffer)
    };

    WARN("LOOK actual buffer size: " << buffer->size());

    JobGgmlTensorAllocator allocator{buffer};

    REQUIRE(allocator.isValid());

    WARN("LOOK allocator alignment: " << allocator.alignment());
    WARN("LOOK allocator capacity: " << allocator.capacity());

    REQUIRE(allocator.allocate(*tensor) == JobGgmlStatus::Success);
}

TEST_CASE("Tensor allocator reports enough storage for padded tensor allocation",
          "[ggml][allocator][edge][alignment]")
{
    JobGgmlDeviceManager manager;
    REQUIRE(manager.isValid());

    JobGgmlCpu *cpu = manager.cpu();
    REQUIRE(cpu != nullptr);
    REQUIRE(cpu->bufferType() != nullptr);

    JobGgmlBackendBufferType *bufferType = cpu->bufferType();

    auto context = JobGgmlContext::createUniqMetadata(16);
    REQUIRE(context != nullptr);

    auto tensor = context->newTensor1d(JobGgmlType::I32, 70);
    REQUIRE(tensor != nullptr);

    const std::size_t requiredSize = JobGgmlTensorAllocator::requiredBufferSize(*bufferType, *tensor);
    REQUIRE(tensor->byteCount() == 280);
    REQUIRE(tensor->paddedByteCount() == 288);

    REQUIRE(bufferType->allocationSize(*tensor) == 280);

    REQUIRE(requiredSize == 288);
    REQUIRE(requiredSize >= tensor->paddedByteCount());
    REQUIRE(requiredSize >= bufferType->allocationSize(*tensor));

    auto uniqueBuffer = bufferType->allocateBuffer(requiredSize);
    REQUIRE(uniqueBuffer != nullptr);

    JobGgmlBackendBuffer::Ptr buffer{
        std::move(uniqueBuffer)
    };
    REQUIRE(buffer->size() >= requiredSize);

    JobGgmlTensorAllocator allocator{buffer};
    REQUIRE(allocator.isValid());
    REQUIRE(allocator.allocate(*tensor) == JobGgmlStatus::Success);
}