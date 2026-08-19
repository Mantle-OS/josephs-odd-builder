#include <contracts>
#include <fstream>
#include <iostream>
#include <chrono>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <memory>
#include <array>
#include <meta>

#include "obj_concept.h"

#include "ping.h"
#include "pong.h"
#include "ser_obj.h"
#include "tmp_file.h"


enum class LayerType : uint8_t {
    Input       = 0,    // Input.
    Dense       = 1,    // Standard Linear
    SparseMoE   = 2,    // Mixture of Experts
    Attention   = 3,    // Self/Cross Attention
    Embedding   = 4,    // Token -> Vector
    LayerNorm   = 5,    // Normalization
    RMSNorm     = 6,    // LLaMA style norm
    Residual    = 7,    // Add
    LinearLoRA  = 8,    // Add
    Output      = 9,    // Logits
    Abstract     = 254  // Unknown slash bad layer type
};


enum class ActivationType : uint8_t {
    Identity    = 0,   // f(x) = x
    Sigmoid     = 1,   // squashes to (0, 1)
    Tanh        = 2,   // squashes to (−1, 1)
    HardTanh    = 3,   // piecewise linear cheap approximations
    ///
    ReLU        = 4,    // max(0, x)
    LeakyReLU   = 5,    // x for x > 0, αx for x ≤ 0 (α small, like 0.01)
    PReLU       = 6,    // like Leaky, but α is learned.
    RReLU       = 7,    // α random in training.
    ELU         = 8,    // exponential on the negative side
    SELU        = 9,    // scaled variant designed for self-normalizing networks.
    ///
    GELU        = 10,   // gaussian error linear unit
    AproxGELU   = 11,   // Tanh-based approximation of GELU (faster)
    Swish       = 12,   // x * sigmoid(βx) (β sometimes = 1)
    HSwish      = 13,   // piecewise linear approximation to Swish.
    Mish        = 14,   // x * tanh(softplus(x))
    HMish       = 15,   // piecewise linears mimicking Mish/Swish.
    Softplus    = 16,   // log(1 + exp(x)) (a smooth ReLU).
    ///
    Maxout      = 17,   // f(x) = max(x₁, x₂, …, x_k) across groups of channels.
    //
    GDN         = 18    // generalized divisive normalization
};



struct Packed {
    LayerType      type{LayerType::Dense};
    ActivationType activation{ActivationType::Identity};
    std::uint8_t   _pad[2]{};
    std::uint32_t  inputs{0};
    std::uint32_t  outputs{0};
    std::uint32_t  weightOffset{0};
    std::uint32_t  weightCount{0};
    std::uint32_t  biasOffset{0};
    std::uint32_t  biasCount{0};
    std::uint32_t  auxiliaryData{0};

    static consteval auto reflectedMembers()
    {
        constexpr auto ctx = std::meta::access_context::unchecked();
        return std::define_static_array(std::meta::nonstatic_data_members_of(^^Packed, ctx));
    }

    static consteval std::size_t reflectedMemberCount()
    {
        return reflectedMembers().size();
    }
};

static_assert(sizeof(Packed) == 32);
static_assert(Packed::reflectedMemberCount() == 10);

struct Serializer
{
    static bool save(const Packed &g, const std::string &filename)
    {
        std::ofstream out(filename, std::ios::binary);
        if (!out)
            return false;

        out.write(reinterpret_cast<const char *>(&g), sizeof(Packed));
        return out.good();
    }

    static Packed load(const std::string &filename)
    {
        Packed g{};

        std::ifstream in(filename, std::ios::binary);
        if (!in)
            return {};

        in.read(reinterpret_cast<char *>(&g), sizeof(Packed));
        if (!in || in.gcount() != sizeof(Packed))
            return {};

        return g;
    }
};

class MappedFile
{
    int m_fd{-1};
    void *m_data{nullptr};
    std::size_t m_size{0};

public:
    MappedFile(const std::string &path, std::size_t size) : m_size(size)
    {
        m_fd = ::open(path.c_str(), O_RDWR | O_CREAT, 0644);
        ::ftruncate(m_fd, size);
        m_data = ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, 0);
    }

    ~MappedFile()
    {
        if (m_data)
            ::munmap(m_data, m_size);

        if (m_fd >= 0)
            ::close(m_fd);
    }

    [[nodiscard]] const Packed *asPacked(std::size_t offset = 0) const noexcept
    {
        auto *ptr = const_cast<char *>(static_cast<const char *>(m_data) + offset);
        return std::start_lifetime_as<Packed>(ptr);
    }

    [[nodiscard]] Packed *asPackedMut(std::size_t offset = 0) noexcept
    {
        return std::start_lifetime_as<Packed>(static_cast<char *>(m_data) + offset);
    }
};


template <typename T>
void dumpReflected(const T &obj)
{
    template for (constexpr auto member : T::reflectedMembers()) {
        std::cout << std::meta::identifier_of(member) << ": ";

        using V = std::remove_cvref_t<decltype(obj.[:member:])>;

        if constexpr (std::is_enum_v<V>) {
            using U = std::underlying_type_t<V>;
            std::cout << static_cast<std::uint64_t>(static_cast<U>(obj.[:member:]));
        } else if constexpr (std::is_array_v<V>) {
            std::cout << '[';

            bool first = true;
            for (const auto &value : obj.[:member:]) {
                if (!first)
                    std::cout << ", ";

                std::cout << static_cast<std::uint64_t>(value);
                first = false;
            }

            std::cout << ']';
        } else {
            std::cout << obj.[:member:];
        }

        std::cout << '\n';
    }
}




// int main()
// {
//     const std::string path = "/tmp/job-packed-mmap.bin";
//     constexpr int iterations = 1'000'000;

//     Packed src;
//     src.type = LayerType::Attention;
//     src.activation = ActivationType::GELU;
//     src.inputs = 4096;
//     src.outputs = 8192;
//     src.weightOffset = 128;
//     src.weightCount = 16384;
//     src.biasOffset = 16512;
//     src.biasCount = 4096;
//     src.auxiliaryData = 8;

//     {
//         MappedFile file(path, sizeof(Packed));
//         Packed *mapped = file.asPackedMut();

//         using Clock = std::chrono::high_resolution_clock;

//         const auto writeStart = Clock::now();

//         for (int i = 0; i < iterations; ++i) {
//             *mapped = src;
//             mapped->inputs = static_cast<std::uint32_t>(4096 + (i & 1));

//             asm volatile("" : : "g"(mapped) : "memory");
//         }

//         const auto writeEnd = Clock::now();

//         std::uint64_t checksum = 0;
//         const Packed *read = file.asPacked();

//         const auto readStart = Clock::now();

//         for (int i = 0; i < iterations; ++i) {
//             // asm volatile("" : "+g"(checksum));
//             asm volatile("" : : : "memory");

//             checksum += read->inputs;
//             checksum += read->outputs;
//             checksum += read->weightCount;
//             checksum += read->biasCount;
//             checksum += read->auxiliaryData;
//         }

//         const auto readEnd = Clock::now();

//         contract_assert(read->type == LayerType::Attention);
//         contract_assert(read->activation == ActivationType::GELU);
//         contract_assert(read->inputs == 4097);
//         contract_assert(read->outputs == 8192);
//         contract_assert(read->weightOffset == 128);
//         contract_assert(read->weightCount == 16384);
//         contract_assert(read->biasOffset == 16512);
//         contract_assert(read->biasCount == 4096);
//         contract_assert(read->auxiliaryData == 8);
//         contract_assert(checksum != 0);

//         std::cout << "\nreflected Packed:\n";
//         dumpReflected(*read);

//         const auto writeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(writeEnd - writeStart).count();
//         const auto readNs = std::chrono::duration_cast<std::chrono::nanoseconds>(readEnd - readStart).count();

//         std::cout << "\nstructure size: " << sizeof(Packed) << " bytes\n";
//         std::cout << iterations << " mapped writes: " << writeNs << " ns\n";
//         std::cout << iterations << " mapped reads:  " << readNs << " ns\n";
//         std::cout << "write avg: " << static_cast<double>(writeNs) / iterations << " ns\n";
//         std::cout << "read avg:  " << static_cast<double>(readNs) / iterations << " ns\n";
//         std::cout << "checksum: " << checksum << '\n';
//     }

//     std::remove(path.c_str());

//     return 0;
// }



// int main()
// {
//     Packed src;
//     src.type = LayerType::Attention;
//     src.activation = ActivationType::GELU;
//     src.inputs = 4096;
//     src.outputs = 4096;
//     src.weightOffset = 128;
//     src.weightCount = 16384;
//     src.biasOffset = 16512;
//     src.biasCount = 4096;
//     src.auxiliaryData = 8;

//     static_assert(sizeof(Packed) == 32);
//     static_assert(std::is_trivially_copyable_v<Packed>);
//     static_assert(std::is_standard_layout_v<Packed>);

//     const std::string path = "/tmp/job-packed-test.bin";
//     constexpr int iterations = 100;


//     using Clock = std::chrono::high_resolution_clock;
//     const auto writeStart = Clock::now();
//     for (int i = 0; i < iterations; ++i) {
//         if (!Serializer::save(src, path))
//             return 1;
//     }
//     const auto writeEnd = Clock::now();
//     Packed dst{};
//     const auto readStart = Clock::now();
//     for (int i = 0; i < iterations; ++i)
//         dst = Serializer::load(path);
//     const auto readEnd = Clock::now();

//     contract_assert(dst.type == LayerType::Attention);
//     contract_assert(dst.activation == ActivationType::GELU);
//     contract_assert(dst.inputs == 4096);
//     contract_assert(dst.outputs == 4096);
//     contract_assert(dst.weightOffset == 128);
//     contract_assert(dst.weightCount == 16384);
//     contract_assert(dst.biasOffset == 16512);
//     contract_assert(dst.biasCount == 4096);
//     contract_assert(dst.auxiliaryData == 8);

//     const auto writeUs =
//         std::chrono::duration_cast<std::chrono::microseconds>(writeEnd - writeStart).count();

//     const auto readUs =
//         std::chrono::duration_cast<std::chrono::microseconds>(readEnd - readStart).count();

//     std::cout << "structure size: " << sizeof(Packed) << " bytes\n";
//     std::cout << "100 binary writes: " << writeUs << " us\n";
//     std::cout << "100 binary reads:  " << readUs << " us\n";
//     std::cout << "write avg: " << static_cast<double>(writeUs) / iterations << " us\n";
//     std::cout << "read avg:  " << static_cast<double>(readUs) / iterations << " us\n";

//     std::remove(path.c_str());

//     return 0;
// }

int main()
{
    Pong pong;
    {
        Ping ping;
        connect<&Ping::pingChanged, &Pong::handlePing>(ping, pong);
        for(int i = 0; i <= 10; ++i )
            ping.emit(i);
    }

    return 0;
}
// divide(10, 0);
