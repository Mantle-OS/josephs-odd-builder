#pragma once

#include <cstdlib>   // posix_memalign, free

#include <vector>
#include <limits>
#include <memory>

#include <real_type.h>
namespace job::ai::cords {

template <typename T_IDL, std::size_t Alignment = 64>
class AlignedAllocator {
public:
    using value_type = T_IDL;

    static_assert(Alignment >= alignof(T_IDL), "Alignment must be at least alignof(T_IDL)");
    static_assert((Alignment & (Alignment - 1)) == 0, "Alignment must be a power of two");

    template <class U>
    struct rebind {
        using other = AlignedAllocator<U, Alignment>;
    };

    AlignedAllocator() noexcept = default;

    template <class U>
    AlignedAllocator(const AlignedAllocator<U, Alignment> &) noexcept
    {
    }

    [[nodiscard]] T_IDL *allocate(std::size_t n)
    {
        if (n > std::numeric_limits<std::size_t>::max() / sizeof(T_IDL))
            throw std::bad_alloc();

        const std::size_t bytes = n * sizeof(T_IDL);

        void *ptr = ::operator new(bytes, std::align_val_t{Alignment});
        return reinterpret_cast<T_IDL*>(ptr);
    }

    void deallocate(T_IDL *p, std::size_t) noexcept
    {
        ::operator delete(p, std::align_val_t{Alignment});
    }
};

template <typename T, typename U, std::size_t Alignment>
bool operator==(const AlignedAllocator<T, Alignment> &, const AlignedAllocator<U, Alignment> &) noexcept
{
    return true;
}

template <typename T, typename U, std::size_t Alignment>
bool operator!=(const AlignedAllocator<T, Alignment> &, const AlignedAllocator<U, Alignment> &) noexcept
{
    return false;
}

// Pick your poison: core::real_t or float
using AiWeights = std::vector<core::real_t, AlignedAllocator<core::real_t, 64>>;

} // namespace job::ai::cords
