#include <catch2/catch_test_macros.hpp>

// threading
#include <job_thread_pool.h>
#include <sched/job_fifo_scheduler.h>
#include <utils/job_stencil_grid_2D.h>
#include <utils/job_stencil_grid_3D.h>

using namespace job::threads;

// Conway's Game of Life (2D)
// 1 = Alive, 0 = Dead
using LifeGrid = JobStencilGrid2D<uint8_t>;

auto gameOfLifeKernel(int x, int y, GridReader2D<uint8_t> view) -> uint8_t
{
    int neighbors = 0;
    // 8 neighbor wrapping check
    neighbors += view.at(x - 1, y - 1, BoundaryMode::Wrap);
    neighbors += view.at(x,     y - 1, BoundaryMode::Wrap);
    neighbors += view.at(x + 1, y - 1, BoundaryMode::Wrap);

    neighbors += view.at(x - 1, y,     BoundaryMode::Wrap);
    // skip self
    neighbors += view.at(x + 1, y,     BoundaryMode::Wrap);

    neighbors += view.at(x - 1, y + 1, BoundaryMode::Wrap);
    neighbors += view.at(x,     y + 1, BoundaryMode::Wrap);
    neighbors += view.at(x + 1, y + 1, BoundaryMode::Wrap);

    uint8_t self = view(x, y);

    // Rules
    // Underpopulation: < 2 -> dies
    // Survival: 2 or 3 -> lives
    // Overpopulation: > 3 -> dies
    // Reproduction: exactly 3 -> becomes alive
    if (self == 1)
        return (neighbors == 2 || neighbors == 3) ? 1 : 0;
    else
        return (neighbors == 3) ? 1 : 0;
}

TEST_CASE("Stencil 2D: Game of Life Blinker Oscillator", "[threading][stencil][2d][example]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool = ThreadPool::create(sched, 4);

    // 5x5 grid is enough for a blinker ?
    LifeGrid grid(*pool, 5, 5, 0);

    // start V
    grid.set(2, 1, 1);
    grid.set(2, 2, 1);
    grid.set(2, 3, 1);
    // flip H
    grid.step(gameOfLifeKernel);

    const uint8_t *data = grid.data();
    REQUIRE(data[2 * 5 + 1] == 1);
    REQUIRE(data[2 * 5 + 2] == 1);
    REQUIRE(data[2 * 5 + 3] == 1);

    REQUIRE(data[1 * 5 + 2] == 0);
    REQUIRE(data[3 * 5 + 2] == 0);

    grid.step(gameOfLifeKernel);

    data = grid.data();
    REQUIRE(data[2 * 5 + 1] == 0);
    REQUIRE(data[2 * 5 + 3] == 0);
    REQUIRE(data[1 * 5 + 2] == 1);
    REQUIRE(data[2 * 5 + 2] == 1);
    REQUIRE(data[3 * 5 + 2] == 1);

    pool->shutdown();
}

TEST_CASE("Stencil 2D: double-buffering preserves old state during step", "[threading][stencil][2d][double_buffer]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 1);

    JobStencilGrid2D<int> grid(*pool, 3, 1, 0);

    grid.set(0, 0, 1);

    // Kernel: copy left neighbor (clamped)
    auto shiftRight = [](int x, int y, GridReader2D<int> view) {
        return view.at(x - 1, y, BoundaryMode::Clamp);
    };

    grid.step(shiftRight);
    const int *d1 = grid.data();
    REQUIRE(d1[0] == 1);
    REQUIRE(d1[1] == 1);
    REQUIRE(d1[2] == 0);

    grid.step(shiftRight);
    const int *d2 = grid.data();
    REQUIRE(d2[0] == 1);
    REQUIRE(d2[1] == 1);
    REQUIRE(d2[2] == 1);

    pool->shutdown();
}

TEST_CASE("Stencil 2D: BoundaryMode::Wrap wraps across edges", "[threading][stencil][2d][wrap]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 1);

    JobStencilGrid2D<int> grid(*pool, 3, 1, 0);
    grid.set(0, 0, 1);
    grid.set(2, 0, 2);

    // Read "left neighbor" with wrap
    auto leftWrap = [](int x, int y, GridReader2D<int> view) {
        return view.at(x - 1, y, BoundaryMode::Wrap);
    };

    grid.step(leftWrap);
    const int *res = grid.data();

    REQUIRE(res[0] == 2);
    REQUIRE(res[1] == 1);
    REQUIRE(res[2] == 0);

    pool->shutdown();
}


using HeatGrid = JobStencilGrid3D<float>;
auto heatDiffusionKernel(int x, int y, int z, GridReader3D<float> view) -> float
{
    // 6-neighbor "Von Neumann" neighborhood
    // FixedZero means the edge of the universe is 0 degrees(Dirichlet boundary heat sink)
    float sum = 0.0f;
    sum += view.at(x + 1, y, z, BoundaryMode::FixedZero);
    sum += view.at(x - 1, y, z, BoundaryMode::FixedZero);
    sum += view.at(x, y + 1, z, BoundaryMode::FixedZero);
    sum += view.at(x, y - 1, z, BoundaryMode::FixedZero);
    sum += view.at(x, y, z + 1, BoundaryMode::FixedZero);
    sum += view.at(x, y, z - 1, BoundaryMode::FixedZero);

    return (view(x, y, z) + sum) / 7.0f;
}

TEST_CASE("Stencil 3D: Heat Diffusion dissipates over time", "[threading][stencil][3d][example]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool = ThreadPool::create(sched, 8);

    // 10x10x10 cube
    HeatGrid grid(*pool, 10, 10, 10, 0.0f);

    // hot core
    grid.set(5, 5, 5, 1000.0f);

    grid.step_n(10, heatDiffusionKernel);

    // cooled down
    GridReader3D<float> view(grid.data(), 10, 10, 10);
    float centerTemp = view(5, 5, 5);

    // Neighbors warmed up
    float neighborTemp = view(6, 5, 5);

    REQUIRE(centerTemp < 1000.0f);
    REQUIRE(centerTemp > 0.0f);
    REQUIRE(neighborTemp > 0.0f);

    // Source SHOULD is still hottest
    REQUIRE(centerTemp > neighborTemp);

    pool->shutdown();
}

TEST_CASE("Stencil 3D: BoundaryMode::Clamp reuses edge values", "[threading][stencil][3d][clamp]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 1);

    JobStencilGrid3D<int> grid(*pool, 2, 2, 2, 0);

    // Put a single 42 at corner (0,0,0)
    grid.set(0, 0, 0, 42);

    auto leftClamp3D = [](int x, int y, int z, GridReader3D<int> view) {
        return view.at(x - 1, y, z, BoundaryMode::Clamp);
    };

    grid.step(leftClamp3D);
    const int *d = grid.data();

    REQUIRE(d[0] == 42);
    REQUIRE(d[1] == 42);

    pool->shutdown();
}


TEST_CASE("Stencil 3D: BoundaryMode::Wrap wraps in z dimension", "[threading][stencil][3d][wrap]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool  = ThreadPool::create(sched, 1);

    JobStencilGrid3D<int> grid(*pool, 2, 2, 2, 0);

    // Put a value in the "top" layer z=1
    grid.set(0, 0, 1, 7);

    auto forwardZWrap = [](int x, int y, int z, GridReader3D<int> view) {
        return view.at(x, y, z + 1, BoundaryMode::Wrap);
    };

    grid.step(forwardZWrap);
    const int *d = grid.data();

    REQUIRE(d[0] == 7);
    REQUIRE(d[4] == 0);

    pool->shutdown();
}

/////////////////////////////
// Start edge cases
/////////////////////////////
TEST_CASE("Stencil 2D: BoundaryMode::Clamp operates correctly", "[threading][stencil][edge]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool = ThreadPool::create(sched, 1);

    // 3x3 Grid by 10x
    JobStencilGrid2D<int> grid(*pool, 3, 3);
    int val = 10;
    for(int y=0; y<3; ++y)
        for(int x=0; x<3; ++x, val+=10)
            grid.set(x, y, val);


    // If x=0, Clamp should read x=0 again (self)
    auto leftLooker = [](int x, int y, GridReader2D<int> view) {
        return view.at(x - 1, y, BoundaryMode::Clamp);
    };

    grid.step(leftLooker);

    const int *res = grid.data();
    REQUIRE(res[0] == 10);
    REQUIRE(res[1] == 10);
    REQUIRE(res[2] == 20);

    pool->shutdown();
}

TEST_CASE("Stencil 2D: BoundaryMode::FixedZero returns default constructed value", "[threading][stencil][edge]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool = ThreadPool::create(sched, 1);

    // JobStencilGrid2D<int> grid(*pool, 3, 3, 100);
    JobStencilGrid2D<int> grid(*pool, 3, 3, true, 100);
    auto leftLooker = [](int x, int y, GridReader2D<int> view) {
        return view.at(x - 1, y, BoundaryMode::FixedZero);
    };

    grid.step(leftLooker);

    const int *res = grid.data();
    REQUIRE(res[0] == 0);
    REQUIRE(res[1] == 100); // FAILS NOW

    pool->shutdown();
}

TEST_CASE("Stencil 3D: step_n(0) does nothing", "[threading][stencil][edge]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool = ThreadPool::create(sched, 1);

    JobStencilGrid3D<int> grid(*pool, 4, 4, 4, 5);

    bool ran = false;
    grid.step_n(0, [&](int, int, int, GridReader3D<int>) {
        ran = true;
        return 0;
    });

    REQUIRE(ran == false);
    REQUIRE(grid.data()[0] == 5);

    pool->shutdown();
}

TEST_CASE("Stencil 2D: 0x0 Grid is safe", "[threading][stencil][edge]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool = ThreadPool::create(sched, 1);

    JobStencilGrid2D<int> grid(*pool, 0, 0);

    // 0x0 request is clamped to a valid 1x1 grid and remains safe to step
    grid.step([](int, int, GridReader2D<int>) {
        return 42;
    });

    // make sure the new clamp stuff is really working at 1
    REQUIRE(grid.width() == 1);
    REQUIRE(grid.height() == 1);
    REQUIRE(grid.data()[0] == 42);

    pool->shutdown();
}

// CHECKPOINT v1.0
