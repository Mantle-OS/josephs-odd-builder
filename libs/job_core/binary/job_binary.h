#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "job_obj_concept.h"

namespace job::binary {

class JobBinary
{
public:
    JobBinary() = delete;
    ~JobBinary() = delete;

    JobBinary(const JobBinary &) = delete;
    JobBinary &operator=(const JobBinary &) = delete;
    JobBinary(JobBinary &&) = delete;
    JobBinary &operator=(JobBinary &&) = delete;

    template <typename T>
    [[nodiscard]] static bool toBytes(const T &value, std::vector<std::uint8_t> &output)
    {
        try {
            output.clear();
            write(output, value);
            return true;
        } catch (...) {
            output.clear();
            return false;
        }
    }

    template <typename T>
    [[nodiscard]] static bool fromBytes(std::span<const std::uint8_t> input, T &value)
    {
        try {
            read(input, value);
            return input.empty();
        } catch (...) {
            return false;
        }
    }

    template <typename T>
    static void write(std::vector<std::uint8_t> &output, const T &value)
    {
        using ValueType = std::remove_cvref_t<T>;

        if constexpr (job::core::OptionalType<ValueType>) {
            const bool present = value.has_value();
            write(output, present);

            if (present)
                write(output, *value);
        } else if constexpr (job::core::OwningSmartPointer<ValueType>) {
            const bool present = static_cast<bool>(value);
            write(output, present);

            if (present)
                write(output, *value);
        } else if constexpr (job::core::UnsupportedPersistentPointer<ValueType>) {
            static_assert(job::core::dependentFalseV<ValueType>,
                          "Raw and weak pointers cannot be serialized");
        } else if constexpr (std::same_as<ValueType, std::string>) {
            const std::uint64_t length = value.size();

            write(output, length);

            appendBytes(
                output,
                reinterpret_cast<const std::uint8_t *>(value.data()),
                value.size());
        } else if constexpr (job::core::MapContainer<ValueType>) {
            const std::uint64_t count = value.size();

            write(output, count);

            for (const auto &[key, mapped] : value) {
                write(output, key);
                write(output, mapped);
            }
        } else if constexpr (job::core::PersistentContainer<ValueType>) {
            const std::uint64_t count = value.size();

            write(output, count);

            for (const auto &item : value)
                write(output, item);
        } else if constexpr (job::core::ReflectableContainer<ValueType>) {
            static_assert(job::core::dependentFalseV<ValueType>,
                          "Unsupported container type in binary serialization");
        } else if constexpr (job::core::BaseObjectType<ValueType>) {
            template for (constexpr auto member : job::core::reflectedDataMembersV<ValueType>) {
                if constexpr (job::core::isSerializableMember<member>())
                    write(output, value.[:member:]);
            }
        } else if constexpr (std::is_trivially_copyable_v<ValueType>) {
            appendBytes(
                output,
                reinterpret_cast<const std::uint8_t *>(&value),
                sizeof(ValueType));
        } else {
            static_assert(job::core::dependentFalseV<ValueType>,
                          "Unsupported type in binary serialization");
        }
    }

    template <typename T>
    static void read(std::span<const std::uint8_t> &input, T &value)
    {
        using ValueType = std::remove_cvref_t<T>;

        if constexpr (job::core::OptionalType<ValueType>) {
            bool present = false;
            read(input, present);

            if (present) {
                value.emplace();
                read(input, *value);
            } else {
                value.reset();
            }
        } else if constexpr (job::core::OwningSmartPointer<ValueType>) {
            bool present = false;
            read(input, present);

            if (present) {
                constructPointer(value);
                read(input, *value);
            } else {
                value.reset();
            }
        } else if constexpr (job::core::UnsupportedPersistentPointer<ValueType>) {
            static_assert(job::core::dependentFalseV<ValueType>,
                          "Raw and weak pointers cannot be deserialized");
        } else if constexpr (std::same_as<ValueType, std::string>) {
            std::uint64_t length = 0;
            read(input, length);

            if (input.size() < length)
                throw std::runtime_error("Unexpected buffer EOF reading string");

            value.assign(
                reinterpret_cast<const char *>(input.data()),
                static_cast<std::size_t>(length));

            input = input.subspan(static_cast<std::size_t>(length));
        } else if constexpr (job::core::MapContainer<ValueType>) {
            std::uint64_t count = 0;
            read(input, count);

            value.clear();

            for (std::uint64_t i = 0; i < count; ++i) {
                typename ValueType::key_type key{};
                typename ValueType::mapped_type mapped{};

                read(input, key);
                read(input, mapped);

                value.emplace(std::move(key), std::move(mapped));
            }
        } else if constexpr (job::core::FixedSequenceContainer<ValueType>) {
            std::uint64_t count = 0;
            read(input, count);

            if (count != value.size())
                throw std::runtime_error("Binary fixed array size mismatch");

            for (auto &item : value)
                read(input, item);
        } else if constexpr (job::core::PushBackSequenceContainer<ValueType>) {
            std::uint64_t count = 0;
            read(input, count);

            value.clear();

            for (std::uint64_t i = 0; i < count; ++i) {
                typename ValueType::value_type item{};
                read(input, item);
                value.push_back(std::move(item));
            }
        } else if constexpr (job::core::InsertSequenceContainer<ValueType>) {
            std::uint64_t count = 0;
            read(input, count);

            value.clear();

            for (std::uint64_t i = 0; i < count; ++i) {
                typename ValueType::value_type item{};
                read(input, item);
                value.insert(std::move(item));
            }
        } else if constexpr (job::core::ReflectableContainer<ValueType>) {
            static_assert(job::core::dependentFalseV<ValueType>,
                          "Unsupported container type in binary deserialization");
        } else if constexpr (job::core::BaseObjectType<ValueType>) {
            template for (constexpr auto member : job::core::reflectedDataMembersV<ValueType>) {
                if constexpr (job::core::isSerializableMember<member>())
                    read(input, value.[:member:]);
            }
        } else if constexpr (std::is_trivially_copyable_v<ValueType>) {
            if (input.size() < sizeof(ValueType))
                throw std::runtime_error("Unexpected buffer EOF in binary stream");

            std::memcpy(&value, input.data(), sizeof(ValueType));
            input = input.subspan(sizeof(ValueType));
        } else {
            static_assert(job::core::dependentFalseV<ValueType>,
                          "Unsupported type in binary deserialization");
        }
    }

private:
    static void appendBytes(std::vector<std::uint8_t> &output,
                            const std::uint8_t *data,
                            std::size_t size)
    {
        // for (std::size_t i = 0; i < size; ++i)
            // output.push_back(data[i]);  << you loose bye bye
            output.append_range(std::span<const std::uint8_t>{data, size});

    }

    template <job::core::OwningSmartPointer Pointer>
    static void constructPointer(Pointer &pointer)
    {
        using PointerType = std::remove_cvref_t<Pointer>;
        using ElementType = typename PointerType::element_type;

        if constexpr (job::core::SharedPointer<PointerType>) {
            if constexpr (requires {
                              { ElementType::createShared() } -> std::convertible_to<PointerType>;
                          }) {
                pointer = ElementType::createShared();
            } else {
                pointer = std::make_shared<ElementType>();
            }
        } else if constexpr (job::core::UniquePointer<PointerType>) {
            if constexpr (requires {
                              { ElementType::createUniq() } -> std::convertible_to<PointerType>;
                          }) {
                pointer = ElementType::createUniq();
            } else if constexpr (std::constructible_from<PointerType, ElementType *>) {
                pointer = PointerType(new ElementType{});
            } else {
                static_assert(job::core::dependentFalseV<PointerType>,
                              "Unique pointer type cannot be reconstructed");
            }
        }
    }
};



// JobBinary alpha -> beta  Right now we are pre Alpha

// [ ] BinaryOutputSink abstraction
//     - std::vector<std::uint8_t>
//     - std::inplace_vector
//     - fixed buffer / std::span
//     - network sink
//     - file sink

// [ ] Remove std::vector assumptions from write path
//     - JobBinary should serialize to a sink concept/interface
//     - vector becomes just one sink implementation
//     - append_range-style bulk append remains the preferred path

// [ ] Bulk-copy trivially-copyable sequences
//     - vector<uint32_t>, array<float, N>, etc.
//     - don't recursively serialize every scalar when the entire contiguous
//       sequence can be copied at once
//     - preserve the generic fallback for non-trivial element types

// [ ] Consider size calculation / reservation
//     - precompute encoded size where worthwhile
//     - reserve once for dynamic sinks
//     - particularly useful for large objects / nested containers
//     - don't make fixed-capacity sinks depend on allocation

// [ ] Define binary portability policy
//     - endianness
//     - bool representation
//     - enum representation
//     - integer widths
//     - struct padding must never leak into the format
//     - floating-point assumptions
//     - versioning / format evolution

// [ ] Decide failure model for bounded sinks
//     - inplace_vector capacity exceeded
//     - fixed span exhausted
//     - return failure / diagnostic rather than allocation

// [ ] Keep read side similarly transport-independent
//     - today std::span<const uint8_t> is already a pretty good primitive
//     - later potentially a BinaryInputSource if streaming becomes useful

// JobBinary alpha -> beta part 2

// [ ] 1. Harden container counts before allocation/iteration
//     - validate uint64_t -> size_t conversion
//     - impose a maximum/container policy
//     - validate against remaining payload where mathematically possible
//     - reserve() when supported
//     - avoid attacker-controlled huge loops / allocations

// [ ] 2. Bulk serialize contiguous scalar sequences
//     - vector<uint32_t>, vector<float>, array<T,N>, etc.
//     - one append_range() instead of N recursive write() calls
//     - one memcpy/read instead of N read() calls
//     - retain recursive fallback for non-bitwise-wire-safe elements

// [ ] 3. Kill generic "trivially_copyable == wire safe"
//     - padding bytes
//     - indeterminate bytes
//     - embedded pointers
//     - ABI/layout differences
//     - deterministic output
//     - raw copying only for explicitly supported wire scalars/types

// [ ] 4. Define canonical scalar wire representation
//     - byte order
//     - bool encoding
//     - enums
//     - integers
//     - floats
//     - fixed widths
//     - signed representation assumptions

// [ ] 5. BinaryOutputSink abstraction
//     - std::vector<uint8_t>
//     - std::inplace_vector
//     - bounded/fixed buffer
//     - file
//     - network
//     - remove std::vector dependency from JobBinary::write()

// [ ] 6. Bounded sink failure semantics
//     - capacity exhaustion
//     - overflow
//     - propagate through current JobBinary bool/error boundary
//     - don't turn fixed-buffer serialization into accidental allocation

// [ ] 7. Encoded-size / reservation support
//     - optional preflight encodedSize(value)
//     - dynamic sinks can reserve once
//     - bounded sinks can reject before partially writing where useful

// [ ] 8. Input abstraction only if needed
//     - span<const uint8_t> is already excellent for memory buffers
//     - streaming BinaryInputSource only when file/network streaming actually needs it

// [ ] 9. Format/version evolution
//     - magic/version if this becomes persistent/interchange format
//     - schema compatibility policy
//     - malformed/truncated/oversized input diagnostics



} // namespace job::binary