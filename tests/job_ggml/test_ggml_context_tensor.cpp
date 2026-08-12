#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <cstddef>
#include <cstdint>
#include <string>


#include <ggml.h>

#include <job_ggml_cgraph.h>
#include <job_ggml_context.h>
#include <job_ggml_enums.h>
#include <job_ggml_init_params.h>
#include <job_ggml_tensor.h>
#include <job_ggml_tensor_batch.h>
#include <job_ggml_tensor_data.h>
#include <job_ggml_tensor_extents.h>
#include <job_ggml_tensor_fiber.h>
#include <job_ggml_tensor_layout.h>
#include <job_ggml_tensor_matrix.h>
#include <job_ggml_tensor_operation.h>
#include <job_ggml_tensor_shapes.h>
#include <job_ggml_tensor_view.h>
#include <job_ggml_tensor_volume.h>

#include <job_ggml_tensor_op.h>

// #include "test_ggml_utils.h"
using namespace job::ggml;
using Catch::Approx;

// ============================================================================
// Block one: usage / examples
// ============================================================================
TEST_CASE("Context creates inspectable tensors of every supported rank", "[ggml][context_tensor][usage][rank]")
{
    constexpr std::size_t payloadBytes =
        8 * sizeof(float) +
        4 * 3 * sizeof(float) +
        4 * 3 * 2 * sizeof(float) +
        4 * 3 * 2 * 5 * sizeof(float);

    auto initParams = JobGgmlInitParams::createUniqMetadataFor(4, GGML_DEFAULT_GRAPH_SIZE, false, payloadBytes, false);
    auto context = JobGgmlContext::createUniq(*initParams);

    REQUIRE(context != nullptr);
    REQUIRE(context->isValid());
    REQUIRE_FALSE(context->noAlloc());

    auto fiber  = context->newTensor1d(JobGgmlType::F32, 8);
    auto matrix = context->newTensor2d(JobGgmlType::F32, 4, 3);
    auto volume = context->newTensor3d(JobGgmlType::F32, 4, 3, 2);
    auto batch  = context->newTensor4d(JobGgmlType::F32, 4, 3, 2, 5);

    REQUIRE(fiber != nullptr);
    REQUIRE(matrix != nullptr);
    REQUIRE(volume != nullptr);
    REQUIRE(batch != nullptr);

    REQUIRE(fiber->isValid());
    REQUIRE(matrix->isValid());
    REQUIRE(volume->isValid());
    REQUIRE(batch->isValid());

    SECTION("rank one tensor")
    {
        REQUIRE(fiber->rank() == 1);
        REQUIRE(fiber->extent(0) == 8);
        REQUIRE(fiber->elementCount() == 8);

        REQUIRE(fiber->isVector());
        REQUIRE_FALSE(fiber->isMatrix());
        REQUIRE_FALSE(fiber->isThreeDimensional());
        REQUIRE_FALSE(fiber->isFourDimensional());

        auto rankObject = fiber->asFiber();

        REQUIRE(rankObject != nullptr);
        REQUIRE(rankObject->isValid());
        REQUIRE(rankObject->rank() == 1);
        REQUIRE(rankObject->extent(0) == 8);
    }

    SECTION("rank two tensor")
    {
        REQUIRE(matrix->rank() == 2);
        REQUIRE(matrix->extent(0) == 4);
        REQUIRE(matrix->extent(1) == 3);
        REQUIRE(matrix->elementCount() == 12);

        REQUIRE(matrix->isMatrix());
        REQUIRE_FALSE(matrix->isVector());
        REQUIRE_FALSE(matrix->isThreeDimensional());
        REQUIRE_FALSE(matrix->isFourDimensional());

        auto rankObject = matrix->asMatrix();

        REQUIRE(rankObject != nullptr);
        REQUIRE(rankObject->isValid());
        REQUIRE(rankObject->rank() == 2);
        REQUIRE(rankObject->extent(0) == 4);
        REQUIRE(rankObject->extent(1) == 3);
    }

    SECTION("rank three tensor")
    {
        REQUIRE(volume->rank() == 3);
        REQUIRE(volume->extent(0) == 4);
        REQUIRE(volume->extent(1) == 3);
        REQUIRE(volume->extent(2) == 2);
        REQUIRE(volume->elementCount() == 24);

        REQUIRE(volume->isThreeDimensional());
        REQUIRE_FALSE(volume->isVector());
        REQUIRE_FALSE(volume->isMatrix());
        REQUIRE_FALSE(volume->isFourDimensional());

        auto rankObject = volume->asVolume();

        REQUIRE(rankObject != nullptr);
        REQUIRE(rankObject->isValid());
        REQUIRE(rankObject->rank() == 3);
    }

    SECTION("rank four tensor")
    {
        REQUIRE(batch->rank() == 4);
        REQUIRE(batch->extent(0) == 4);
        REQUIRE(batch->extent(1) == 3);
        REQUIRE(batch->extent(2) == 2);
        REQUIRE(batch->extent(3) == 5);
        REQUIRE(batch->elementCount() == 120);

        REQUIRE(batch->isFourDimensional());
        REQUIRE_FALSE(batch->isVector());
        REQUIRE_FALSE(batch->isMatrix());
        REQUIRE_FALSE(batch->isThreeDimensional());

        auto rankObject = batch->asBatch();

        REQUIRE(rankObject != nullptr);
        REQUIRE(rankObject->isValid());
        REQUIRE(rankObject->rank() == 4);
    }
}

TEST_CASE("Tensor exposes type shape stride and layout inspection", "[ggml][context_tensor][usage][inspection]")
{
    constexpr std::size_t payloadBytes = 4 * 3 * sizeof(float);
    auto initParams = JobGgmlInitParams::createUniqMetadataFor(1, GGML_DEFAULT_GRAPH_SIZE, false, payloadBytes, false);
    auto context = JobGgmlContext::createUniq(*initParams);
    auto tensor = context->newTensor2d(JobGgmlType::F32, 4, 3);

    REQUIRE(tensor != nullptr);
    REQUIRE(tensor->isValid());

    REQUIRE(tensor->extents() != nullptr);
    REQUIRE(tensor->layout() != nullptr);
    REQUIRE(tensor->data() != nullptr);
    REQUIRE(tensor->operation() != nullptr);
    REQUIRE(tensor->view() != nullptr);

    REQUIRE(tensor->type() == JobGgmlType::F32);
    REQUIRE(toGgmlType(tensor->type()) == GGML_TYPE_F32);
    REQUIRE(std::string{tensor->typeName()} == "f32");
    REQUIRE_FALSE(tensor->isQuantized());

    REQUIRE(tensor->rank() == 2);
    REQUIRE(tensor->extent(0) == 4);
    REQUIRE(tensor->extent(1) == 3);
    REQUIRE(tensor->extent(2) == 1);
    REQUIRE(tensor->extent(3) == 1);

    REQUIRE(tensor->stride(0) == sizeof(float));
    REQUIRE(tensor->stride(1) == 4 * sizeof(float));

    REQUIRE(tensor->elementCount() == 12);
    REQUIRE(tensor->byteCount() == 12 * sizeof(float));
    REQUIRE(tensor->paddedByteCount() >= tensor->byteCount());

    REQUIRE(tensor->isContiguous());
    REQUIRE(tensor->isContiguouslyAllocated());
    REQUIRE_FALSE(tensor->isTransposed());
    REQUIRE_FALSE(tensor->isPermuted());
    REQUIRE_FALSE(tensor->isStrided());

    REQUIRE(tensor->layoutType() == JobGgmlTensorLayoutType::Contiguous );
    REQUIRE(tensor->layout()->hasSameShape(  *tensor->layout() ) );
    REQUIRE(tensor->layout()->hasSameStride( *tensor->layout() ) );
    REQUIRE(tensor->layout()->hasSameLayout( *tensor->layout() ) );
    REQUIRE(tensor->extents()->ne0() == 4 );
    REQUIRE(tensor->extents()->ne1() == 3 );
    REQUIRE(tensor->extents()->rowCount() == 3 );
}

TEST_CASE("Tensor host data supports fill flat indexing and coordinates", "[ggml][context_tensor][usage][data]")
{
    constexpr std::size_t payloadBytes = 3 * 2 * sizeof(float) + 4 * sizeof(std::int32_t);
    auto initParams = JobGgmlInitParams::createUniqMetadataFor(2, GGML_DEFAULT_GRAPH_SIZE, false, payloadBytes, false);
    auto context = JobGgmlContext::createUniq(*initParams);
    auto floats = context->newTensor2d(JobGgmlType::F32, 3, 2);
    auto integers = context->newTensor1d(JobGgmlType::I32, 4);

    REQUIRE(floats != nullptr);
    REQUIRE(integers != nullptr);

    REQUIRE(floats->data() != nullptr);
    REQUIRE(integers->data() != nullptr);

    REQUIRE(floats->data()->isHostAccessible());
    REQUIRE(integers->data()->isHostAccessible());

    REQUIRE(floats->hasData());
    REQUIRE(integers->hasData());

    floats->data()->fillF32(2.5f);

    for (std::int64_t index = 0; index < floats->elementCount(); ++index)
        REQUIRE( floats->data()->valueF32(index) == Approx(2.5f) );

    floats->data()->setValueF32( 1, 7.25f );
    REQUIRE( floats->data()->valueF32(1) == Approx(7.25f) );

    floats->data()->setValueF32( 2, 1, 0, 0, 13.5f );
    REQUIRE( floats->data()->valueF32( 2, 1, 0, 0 ) == Approx(13.5f) );

    integers->data()->fillI32(42);

    for (std::int64_t index = 0; index < integers->elementCount(); ++index)
        REQUIRE( integers->data()->valueI32(index) == 42 );

    integers->data()->setValueI32( 2, -17 );
    REQUIRE( integers->data()->valueI32(2) == -17 );

    integers->data()->setValueI32( 3, 0, 0, 0, 99 );
    REQUIRE(integers->data()->valueI32(3, 0, 0, 0 ) == 99);
}

TEST_CASE("Context names looks up and iterates tensors", "[ggml][context_tensor][usage][lookup]")
{
    constexpr std::size_t payloadBytes =
        4 * sizeof(float) +
        2 * 2 * sizeof(float) +
        2 * 2 * 2 * sizeof(std::int32_t);
    auto initParams = JobGgmlInitParams::createUniqMetadataFor(3, GGML_DEFAULT_GRAPH_SIZE, false, payloadBytes, false);
    auto context = JobGgmlContext::createUniq(*initParams);
    auto first   = context->newTensor1d(JobGgmlType::F32, 4);
    auto second  = context->newTensor2d(JobGgmlType::F32, 2, 2);
    auto third   = context->newTensor3d(JobGgmlType::I32, 2, 2, 2);

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    REQUIRE(third != nullptr);

    first->setName("first");
    second->setName("second");
    third->setName("third");

    REQUIRE(first->hasName());
    REQUIRE(second->hasName());
    REQUIRE(third->hasName());

    REQUIRE(first->name() == "first");
    REQUIRE(second->name() == "second");
    REQUIRE(third->name() == "third");

    auto found = context->tensor("second");

    REQUIRE(found != nullptr);
    REQUIRE(found->isValid());
    REQUIRE(found->tensor() == second->tensor());
    REQUIRE(found->name() == "second");

    auto current = context->firstTensor();

    std::size_t visited = 0;

    while (current) {
        REQUIRE(current->isValid());
        ++visited;
        current = context->nextTensor(*current);
    }

    REQUIRE(visited == 3);
}

TEST_CASE("Context duplicates tensors and creates borrowed views", "[ggml][context_tensor][usage][view]")
{
    constexpr std::size_t payloadBytes = 2 * 4 * 3 * sizeof(float);
    auto initParams = JobGgmlInitParams::createUniqMetadataFor(3, GGML_DEFAULT_GRAPH_SIZE, false, payloadBytes, false);
    auto context = JobGgmlContext::createUniq(*initParams);

    auto source = context->newTensor2d(JobGgmlType::F32, 4, 3);

    REQUIRE(source != nullptr);

    source->setName("source");
    source->data()->fillF32(3.0f);

    auto duplicate = context->duplicateTensor( *source );

    REQUIRE(duplicate != nullptr);
    REQUIRE(duplicate->isValid());

    REQUIRE( duplicate->tensor() != source->tensor() );
    REQUIRE( duplicate->hasSameShape( *source ) );

    REQUIRE( duplicate->hasSameStride( *source ) );

    REQUIRE( duplicate->hasSameLayout( *source ) );

    auto view = context->viewTensor( *source );
    REQUIRE(view != nullptr);
    REQUIRE(view->isValid());

    REQUIRE(view->isView());
    REQUIRE(view->viewSource() == source->tensor());
    REQUIRE(view->rootViewSource() == source->tensor());
    REQUIRE(view->viewOffset() == 0);

    REQUIRE( view->hasSameShape( *source ) );
    REQUIRE( view->hasSameStride( *source ) );
}

TEST_CASE("Tensor flags describe graph roles", "[ggml][context_tensor][usage][flags]")
{
    constexpr std::size_t payloadBytes = 4 * sizeof(float) + 4 * sizeof(float) + sizeof(float);
    auto initParams = JobGgmlInitParams::createUniqMetadataFor(3, GGML_DEFAULT_GRAPH_SIZE, false, payloadBytes, false);
    auto context = JobGgmlContext::createUniq(*initParams);

    auto input = context->newTensor1d(JobGgmlType::F32, 4);

    auto parameter = context->newTensor1d(JobGgmlType::F32, 4);
    auto loss = context->newTensor1d(JobGgmlType::F32, 1);

    REQUIRE(input != nullptr);
    REQUIRE(parameter != nullptr);
    REQUIRE(loss != nullptr);

    REQUIRE(input->data() != nullptr);
    REQUIRE(parameter->data() != nullptr);
    REQUIRE(loss->data() != nullptr);

    REQUIRE_FALSE(input->isInput());
    REQUIRE_FALSE(input->isOutput());
    REQUIRE_FALSE(input->isCompute());

    input->data()->addFlag(JobGgmlTensorFlag::Input);
    input->data()->addFlag(JobGgmlTensorFlag::Output);
    input->data()->addFlag(JobGgmlTensorFlag::Compute);
    parameter->data()->addFlag(JobGgmlTensorFlag::Param);
    loss->data()->addFlag(JobGgmlTensorFlag::Loss);

    REQUIRE(input->isInput());
    REQUIRE(input->isOutput());
    REQUIRE(input->isCompute());

    REQUIRE(parameter->isParameter());

    REQUIRE(loss->isLoss());
    REQUIRE(loss->isScalar());

    input->data()->removeFlag(JobGgmlTensorFlag::Output);

    REQUIRE_FALSE(input->isOutput());
    REQUIRE(input->isInput());
    REQUIRE(input->isCompute());

    REQUIRE(parameter->isParameter());
    REQUIRE(loss->isLoss());
}

TEST_CASE("Tensor operation inspection describes a simple addition", "[ggml][context_tensor][usage][operation]")
{
    constexpr std::size_t payloadBytes = 3 * 4 * sizeof(float);
    auto initParams = JobGgmlInitParams::createUniqMetadataFor(3, GGML_DEFAULT_GRAPH_SIZE, false, payloadBytes, false);
    auto context = JobGgmlContext::createUniq(*initParams);
    auto left = context->newTensor1d(JobGgmlType::F32, 4);
    auto right = context->newTensor1d(JobGgmlType::F32, 4);

    REQUIRE(left != nullptr);
    REQUIRE(right != nullptr);

    auto op = JobGgmlTensorOp::createUniq(left->tensor(), context.get());
    REQUIRE(op != nullptr);
    REQUIRE(op->isValid());

    auto result = op->add(*right);
    REQUIRE(result != nullptr);
    REQUIRE(result->isValid());

    REQUIRE(result->hasOperation());
    REQUIRE(result->tensorOperation() == JobGgmlOp::Add);
    REQUIRE(result->sourceCount() == 2);

    REQUIRE(result->operation()->source(0) == left->tensor());
    REQUIRE(result->operation()->source(1) == right->tensor());
    REQUIRE(result->operation()->hasSource( left->tensor()));
    REQUIRE(result->operation()->hasSource( right->tensor()));
}

TEST_CASE("Semantic tensor shapes describe logical model dimensions", "[ggml][context_tensor][usage][semantic_shape]")
{
    const JobGgmlBSShape tokens{ 2, 128 };
    REQUIRE(tokens.isValid());
    REQUIRE(tokens.elementCount() == 256);

    const JobGgmlBSDShape hiddenStates{ 2, 128, 768 };
    REQUIRE(hiddenStates.isValid());
    REQUIRE(hiddenStates.vectorsPerBatch() == 128);
    REQUIRE(hiddenStates.elementCount() == 2 * 128 * 768);

    const JobGgmlBSHDShape splitHeads{ 2, 128, 12, 64 };
    REQUIRE(splitHeads.isValid());
    REQUIRE(splitHeads.modelDimension() == 768);

    const JobGgmlBHSSShape attentionScores{ 2, 12, 128, 128 };
    REQUIRE(attentionScores.isValid());
    REQUIRE(attentionScores.isSquareAttention());

    const JobGgmlLinearShape projection{ 768, 768 };
    REQUIRE(projection.isValid());
    REQUIRE(projection.isSquare());

    const JobGgmlBCHWShape imageBatch{ 2, 4, 64, 64 };
    REQUIRE(imageBatch.isValid());
    REQUIRE(imageBatch.spatialSize() == 4096);
    REQUIRE(imageBatch.elementsPerBatch() == 16384);
    REQUIRE(imageBatch.elementCount() == 32768);
}

// ============================================================================
// Block two: edge cases / invariants
// ============================================================================

TEST_CASE("Context rejects invalid tensor dimensions", "[ggml][context_tensor][edge][creation]")
{
    auto initParams = JobGgmlInitParams::createUniqMetadataFor(1, GGML_DEFAULT_GRAPH_SIZE, false, 0, false);
    auto context = JobGgmlContext::createUniq(*initParams);

    REQUIRE_THROWS_AS(context->newTensor1d(JobGgmlType::F32, 0 ), std::invalid_argument);
    REQUIRE_THROWS_AS(context->newTensor2d(JobGgmlType::F32, 4, 0 ), std::invalid_argument);
    REQUIRE_THROWS_AS(context->newTensor3d(JobGgmlType::F32, 4, -1, 2 ), std::invalid_argument);
    REQUIRE_THROWS_AS(context->newTensor4d(JobGgmlType::F32, 4, 3, 2, 0 ), std::invalid_argument);

    const std::int64_t extents[]{ 4, 0 };
    REQUIRE_THROWS_AS(context->newTensor( JobGgmlType::F32, 2, extents ), std::invalid_argument);
}

TEST_CASE( "Context lookup rejects empty and missing names", "[ggml][context_tensor][edge][lookup]")
{
    constexpr std::size_t payloadBytes = 4 * sizeof(float);
    auto initParams = JobGgmlInitParams::createUniqMetadataFor(1, GGML_DEFAULT_GRAPH_SIZE, false, payloadBytes, false);
    auto context = JobGgmlContext::createUniq(*initParams);
    auto tensor = context->newTensor1d(JobGgmlType::F32, 4);
    REQUIRE(tensor != nullptr);
    tensor->setName("present");

    REQUIRE(context->tensor("") == nullptr);
    REQUIRE(context->tensor("missing") == nullptr);
    REQUIRE(context->tensor("present") != nullptr);
}

TEST_CASE("Tensor rejects out of range host data access", "[ggml][context_tensor][edge][data]")
{
    constexpr std::size_t payloadBytes = 3 * 2 * sizeof(float) + 4 * sizeof(std::int32_t);
    auto initParams = JobGgmlInitParams::createUniqMetadataFor(2, GGML_DEFAULT_GRAPH_SIZE, false, payloadBytes, false);
    auto context = JobGgmlContext::createUniq(*initParams);
    auto floats = context->newTensor2d(JobGgmlType::F32, 3, 2);
    auto integers = context->newTensor1d(JobGgmlType::I32, 4);

    REQUIRE(floats != nullptr);
    REQUIRE(integers != nullptr);

    REQUIRE_THROWS_AS(floats->data()->valueF32(-1), std::out_of_range);
    REQUIRE_THROWS_AS(floats->data()->valueF32(6),  std::out_of_range);
    REQUIRE_THROWS_AS(floats->data()->setValueF32(3, 0, 0, 0, 1.0f ), std::out_of_range);
    REQUIRE_THROWS_AS( floats->data()->setValueF32(0, 2, 0, 0, 1.0f ), std::out_of_range);
    REQUIRE_THROWS_AS(integers->data()->valueI32(4), std::out_of_range);
    REQUIRE_THROWS_AS(integers->data()->setValueI32( -1, 5 ), std::out_of_range);
}

TEST_CASE("Tensor shape comparisons distinguish compatible tensors", "[ggml][context_tensor][edge][shape]")
{
    constexpr std::size_t payloadBytes =
        4 * 3 * sizeof(float) +
        4 * 3 * sizeof(float) +
        5 * 3 * sizeof(float) +
        1 * 3 * sizeof(float);
    auto initParams = JobGgmlInitParams::createUniqMetadataFor(4, GGML_DEFAULT_GRAPH_SIZE, false, payloadBytes, false);
    auto context = JobGgmlContext::createUniq(*initParams);

    auto first = context->newTensor2d(JobGgmlType::F32, 4, 3);
    auto same = context->newTensor2d(JobGgmlType::F32, 4, 3);
    auto different = context->newTensor2d(JobGgmlType::F32, 5, 3);
    auto repeatSource = context->newTensor2d(JobGgmlType::F32, 1, 3);

    REQUIRE(first != nullptr);
    REQUIRE(same != nullptr);
    REQUIRE(different != nullptr);
    REQUIRE(repeatSource != nullptr);

    REQUIRE(first->hasSameShape(*same));
    REQUIRE(first->hasSameStride(*same));
    REQUIRE(first->hasSameLayout(*same));
    REQUIRE_FALSE(first->hasSameShape(*different));
    REQUIRE(repeatSource->canRepeatTo(*first));
}

TEST_CASE("Context reset releases its internal object arena", "[ggml][context_tensor][edge][reset]")
{
    constexpr std::size_t payloadBytes = 64 * sizeof(float) + 8 * 8 * sizeof(float);
    auto initParams = JobGgmlInitParams::createUniqMetadataFor(2, GGML_DEFAULT_GRAPH_SIZE, false, payloadBytes, false);
    auto context = JobGgmlContext::createUniq(*initParams);

    auto first = context->newTensor1d(JobGgmlType::F32, 64);
    auto second = context->newTensor2d(JobGgmlType::F32, 8, 8);

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);

    REQUIRE(context->usedMemory() > 0);

    // NOTE : Tensor wrappers borrow native objects owned by the context. Do not use those wrappers after reset().
    first.reset();
    second.reset();

    context->reset();

    REQUIRE(context->usedMemory() == 0);
    REQUIRE(context->firstTensor() == nullptr);
}

TEST_CASE("Rank-specific tensor conversion accepts only matching ranks", "[ggml][context_tensor][edge][rank]")
{
    constexpr std::size_t payloadBytes =
        4 * sizeof(float) +
        4 * 3 * sizeof(float) +
        4 * 3 * 2 * sizeof(float) +
        4 * 3 * 2 * 2 * sizeof(float);
    auto initParams = JobGgmlInitParams::createUniqMetadataFor(4, GGML_DEFAULT_GRAPH_SIZE, false, payloadBytes, false);
    auto context = JobGgmlContext::createUniq(*initParams);

    auto fiber = context->newTensor1d(JobGgmlType::F32, 4);
    auto matrix = context->newTensor2d(JobGgmlType::F32, 4, 3);
    auto volume = context->newTensor3d(JobGgmlType::F32, 4, 3, 2);
    auto batch = context->newTensor4d(JobGgmlType::F32, 4, 3, 2, 2);

    REQUIRE(fiber != nullptr);
    REQUIRE(matrix != nullptr);
    REQUIRE(volume != nullptr);
    REQUIRE(batch != nullptr);

    REQUIRE(fiber->rank() == 1);
    REQUIRE(matrix->rank() == 2);
    REQUIRE(volume->rank() == 3);
    REQUIRE(batch->rank() == 4);

    REQUIRE(fiber->asFiber() != nullptr);
    REQUIRE(matrix->asMatrix() != nullptr);
    REQUIRE(volume->asVolume() != nullptr);
    REQUIRE(batch->asBatch() != nullptr);

    REQUIRE(fiber->asMatrix() == nullptr);
    REQUIRE(matrix->asVolume() == nullptr);
    REQUIRE(volume->asBatch() == nullptr);
    REQUIRE(batch->asFiber() == nullptr);
}

TEST_CASE("GGML collapses trailing singleton tensor dimensions", "[ggml][context_tensor][edge][rank][singleton]")
{
    constexpr std::size_t payloadBytes = 4 * 3 * 2 * sizeof(float);
    auto initParams = JobGgmlInitParams::createUniqMetadataFor(1, GGML_DEFAULT_GRAPH_SIZE, false, payloadBytes, false);
    auto context = JobGgmlContext::createUniq(*initParams);
    auto tensor = context->newTensor4d(JobGgmlType::F32, 4, 3, 2, 1);

    REQUIRE(tensor != nullptr);

    REQUIRE(tensor->extent(3) == 1);
    REQUIRE(tensor->rank() == 3);

    REQUIRE(tensor->asVolume() != nullptr);
    REQUIRE(tensor->asBatch() == nullptr);
}

// ============================================================================
// Block three: benchmarks / stress
// ============================================================================
#ifdef JOB_TEST_BENCHMARKS

TEST_CASE( "Context tensor creation performance", "[ggml][context_tensor][benchmark][creation]" )
{
    BENCHMARK("create one context and tensor") {
        constexpr std::size_t payloadBytes = 64 * 64 * sizeof(float);
        auto initParams = JobGgmlInitParams::createUniqMetadataFor(1, GGML_DEFAULT_GRAPH_SIZE, false, payloadBytes, false);
        auto context = JobGgmlContext::createUniq(*initParams);
        return context->newTensor2d(JobGgmlType::F32, 64, 64);
    };
}

TEST_CASE("Tensor metadata inspection performance", "[ggml][context_tensor][benchmark][inspection]")
{
    auto initParams = JobGgmlInitParams::createUniqMetadataFor(1);
    auto context = JobGgmlContext::createUniq(*initParams);
    auto tensor = context->newTensor4d(JobGgmlType::F32, 64, 32, 8, 2);

    REQUIRE(tensor != nullptr);
    REQUIRE_FALSE(tensor->hasData());

    BENCHMARK("inspect tensor metadata") {
        return tensor->elementCount() +
               static_cast<std::int64_t>(tensor->byteCount()) +
               tensor->extent(0) +
               tensor->extent(1) +
               tensor->extent(2) +
               tensor->extent(3);
    };
}

TEST_CASE("Tensor host indexed access performance", "[ggml][context_tensor][benchmark][data]")
{
    constexpr std::size_t elementCount = 1024;
    constexpr std::size_t payloadBytes = elementCount * sizeof(float);

    auto initParams = JobGgmlInitParams::createUniqMetadataFor(1, GGML_DEFAULT_GRAPH_SIZE, false, payloadBytes, false);
    auto context = JobGgmlContext::createUniq(*initParams);
    auto tensor = context->newTensor1d(JobGgmlType::F32, static_cast<std::int64_t>(elementCount));

    REQUIRE(tensor != nullptr);
    REQUIRE(tensor->data() != nullptr);
    REQUIRE(tensor->data()->isHostAccessible());
    REQUIRE(tensor->hasData());

    tensor->data()->fillF32(1.0f);

    BENCHMARK("read one host tensor element") {
        return tensor->data()->valueF32(511);
    };
}

#endif

