#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

#include <job_render_application.h>

using namespace std::chrono_literals;

namespace job::render::tests {
using Pixel    =  std::array<std::byte, 4>;
using Channel  =  std::byte;

inline constexpr Pixel kWhite{
    Channel{0xFF},
    Channel{0xFF},
    Channel{0xFF},
    Channel{0xFF}
};

inline constexpr Pixel kBlack{
    Channel{0x00},
    Channel{0x00},
    Channel{0x00},
    Channel{0xFF}
};

inline constexpr Pixel kRed{
    Channel{0x00},
    Channel{0x00},
    Channel{0xFF},
    Channel{0xFF}
};

inline constexpr Pixel kGreen{
    Channel{0x00},
    Channel{0xFF},
    Channel{0x00},
    Channel{0xFF}
};

inline constexpr Pixel kBlue{
    Channel{0xFF},
    Channel{0x00},
    Channel{0x00},
    Channel{0xFF}
};

void fill(job::render::JobKmsSimpleSurface &surface, const Pixel &pixel)
{
    auto data = surface.data();

    const std::size_t width  = surface.width();
    const std::size_t height = surface.height();
    const std::size_t stride = surface.stride();

    for (std::size_t y = 0; y < height; ++y) {
        std::byte *row = data.data() + (y * stride);

        for (std::size_t x = 0; x < width; ++x) {
            const std::size_t offset = x * pixel.size();

            row[offset + 0] = pixel[0];
            row[offset + 1] = pixel[1];
            row[offset + 2] = pixel[2];
            row[offset + 3] = pixel[3];
        }
    }
}

}  // namespace job::render::tests

int main(int argc, char *argv[])
{
    std::vector<std::string> arguments;
    arguments.reserve(static_cast<std::size_t>(argc) + 2);

    arguments.emplace_back(argv[0]);

    for (int i = 1; i < argc; ++i)
        arguments.emplace_back(argv[i]);

    arguments.emplace_back("--backend");
    arguments.emplace_back("kms-simple");

    std::vector<char *> argumentPointers;
    argumentPointers.reserve(arguments.size());

    for (auto &argument : arguments)
        argumentPointers.push_back(argument.data());

    auto app = job::render::JobRenderApplication::createUniq(static_cast<int>(argumentPointers.size()), argumentPointers.data());
    if (!app->start())
        return EXIT_FAILURE;

    auto *backend = app->kmsSimpleBackend();
    if (!backend)
        return EXIT_FAILURE;

    auto *surface = backend->surface();
    if (!surface)
        return EXIT_FAILURE;

    constexpr std::array colors {
        job::render::tests::kWhite,
        job::render::tests::kBlack,
        job::render::tests::kRed,
        job::render::tests::kGreen,
        job::render::tests::kBlue
    };
    std::jthread timer([&app, backend, surface, &colors]() {
        for (const auto &color : colors) {
            job::render::tests::fill(*surface, color);

            if (!backend->present()) {
                app->quit(EXIT_FAILURE);
                return;
            }

            std::this_thread::sleep_for(1s);
        }

        app->quit(EXIT_SUCCESS);
    });

    return app->exec();
}