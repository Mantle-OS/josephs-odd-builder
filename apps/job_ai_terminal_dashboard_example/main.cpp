#include <thread>
#include <atomic>
#include <vector>
#include <list>
#include <cmath>
#include <mutex>
#include <sstream>
#include <iomanip>

// IO & TUI
#include <io_factory.h>
#include <color_utils.h>
#include <job_tui_core_application.h>
#include <containers/window.h>
#include <layout/grid_layout.h>
#include <layout/linear_layout.h>
#include <elements/rectangle.h>
#include <elements/label.h>
#include <elements/button.h>

// AI & Threads
#include <ctx/job_stealing_ctx.h>
#include <es_coach.h>
#include <stencil_adapter.h>
#include <layer_factory.h>

using namespace job::io;
using namespace job::tui;
using namespace job::tui::gui;
using namespace job::ai;
using namespace job::ai::coach;
using namespace job::ai::adapters;

// ---------------------------------------------------------
// The Simulation State
// ---------------------------------------------------------
class SimulationState {
public:
    std::atomic<size_t> generation{0};
    std::atomic<float>  bestFitness{0.0f};
    std::atomic<float>  currentSigma{0.0f};
    std::atomic<bool>   running{false};

    // The "Brain Scan" (Heatmap data)
    // Protected by mutex to prevent tearing during render
    std::mutex mapMutex;
    std::vector<float> heatMap;
    int width{16};
    int height{16};

    void updateMap(const float* data, size_t size) {
        std::lock_guard<std::mutex> lock(mapMutex);
        if (heatMap.size() != size) heatMap.resize(size);
        for(size_t i=0; i<size; ++i) {
            heatMap[i] = data[i];
        }
    }
};

// ---------------------------------------------------------
// The Application
// ---------------------------------------------------------
int main() {
    // 1. Setup TUI IO
    auto dev = IOFactory::createFromType(FactoryType::FILE_STD_OUT, "");
    if (!dev || !dev->openDevice()) return 1;

    auto app = std::make_shared<JobTuiCoreApplication>(dev);
    // Bigger window to fit everything nicely
    auto root = Window::create({0, 0, 80, 30}, "Mantle OS - Neural Physics Dashboard", app.get());

    // -------------------------------------------------
    // UI Layout: Split Left (Heatmap) and Right (Stats)
    // -------------------------------------------------

    // Left Panel: The Brain (Stencil Grid)
    // 16x16 Grid.
    auto brainPanel = JobTuiItem::create({2, 2, 34, 20}, root.get());
    brainPanel->setItemHasContents(true);

    // We keep a Vector for random access in the thread
    std::vector<std::shared_ptr<JobTuiItem>> pixels;
    pixels.reserve(256);

    // Create 256 Rectangles
    for(int i=0; i<256; ++i) {
        // Init black
        auto rect = Rectangle::create({0,0,2,1}, "black", nullptr);
        pixels.push_back(rect);
    }

    // Convert to List for Layout Engine
    std::list<JobTuiItem::Ptr> pixelList(pixels.begin(), pixels.end());

    // Cols=16, Spacing=0 (Tight Grid)
    auto gridLayout = GridLayout::create(brainPanel->boundingRect(), 16, 0, pixelList, brainPanel.get());
    brainPanel->applyLayout();

    // Right Panel: Stats
    auto statsPanel = JobTuiItem::create({40, 2, 35, 20}, root.get());
    statsPanel->setItemHasContents(true);
    auto statsLayout = LinearLayout::create(statsPanel->boundingRect(), statsPanel.get());
    statsLayout->setOrientation(LayoutEngine::Orientation::Vertical);

    auto lblGen   = Label::create({0,0,30,1}, "Generation: 0", statsLayout.get());
    auto lblFit   = Label::create({0,0,30,1}, "Fitness:    0.000", statsLayout.get());

    // Spacer
    Rectangle::create({0,0,30,1}, "default", statsLayout.get());

    auto btnToggle = Button::create({0,0,20,3}, "START SIMULATION", statsLayout.get());

    statsPanel->applyLayout();

    // -------------------------------------------------
    // The AI Backend
    // -------------------------------------------------
    auto simState = std::make_shared<SimulationState>();

    // Background Thread
    std::thread worker([simState, app, pixels, lblGen, lblFit]() {
        job::threads::JobStealerCtx ctx(4); // 4 Cores for Physics

        // Setup Stencil Adapter to generate "Thoughts"
        StencilConfig stencilCfg;
        stencilCfg.steps = 5;
        stencilCfg.diffusionRate = 0.1f;
        stencilCfg.boundary = job::threads::BoundaryMode::Wrap;
        StencilAdapter adapter(stencilCfg);

        // Dummy Genome (We just want to visualize the physics really)
        // We'll treat the Genome weights as the Stencil Grid state
        evo::Genome genome;
        genome.weights.resize(16*16, 0.0f); // 256 floats

        // Coach
        ESConfig coachCfg = CoachPresets::kStandard;
        coachCfg.populationSize = 64;
        coachCfg.sigma = 0.5f; // High noise for visuals
        ESCoach coach(ctx.pool, coachCfg);

        // The Evaluator (Creates a Checkerboard target)
        auto eval = [&](const evo::Genome& g) -> float {
            std::vector<float> state = g.weights; // Copy

            cords::ViewR view(state.data(), cords::ViewR::Extent{1, 256});
            cords::ViewR out(state.data(), cords::ViewR::Extent{1, 256}); // In-place
            cords::AttentionShape shape{1, 256, 1, 1};
            AdapterCtx aCtx;

            adapter.adapt(*ctx.pool, shape, view, view, view, out, aCtx);

            // Goal: Checkerboard pattern
            float error = 0.0f;
            for(int i=0; i<256; ++i) {
                int x = i % 16;
                int y = i / 16;
                float target = ((x+y)%2 == 0) ? 1.0f : 0.0f;
                float diff = state[i] - target;
                error += diff * diff;
            }
            return 1.0f / (1.0f + error);
        };

        while(app->isRunning()) {
            if (simState->running) {
                evo::Genome survivor = coach.coach(genome, eval);

                simState->generation = coach.generation();
                simState->bestFitness = coach.currentBestFitness();

                std::vector<float> displayState = survivor.weights;
                cords::ViewR view(displayState.data(), cords::ViewR::Extent{1, 256});
                cords::ViewR out(displayState.data(), cords::ViewR::Extent{1, 256});
                cords::AttentionShape shape{1, 256, 1, 1};
                AdapterCtx ctxDummy;
                adapter.adapt(*ctx.pool, shape, view, view, view, out, ctxDummy);

                simState->updateMap(displayState.data(), 256);

                {
                    std::stringstream ss;
                    ss << "Generation: " << simState->generation;
                    lblGen->setText(ss.str());

                    ss.str(""); ss << std::fixed << std::setprecision(4) << "Fitness:    " << simState->bestFitness;
                    lblFit->setText(ss.str());

                    std::lock_guard<std::mutex> lock(simState->mapMutex);
                    for(int i=0; i<256; ++i) {
                        float val = simState->heatMap[i];
                        auto rect = std::static_pointer_cast<Rectangle>(pixels[i]);

                        using namespace job::ansi::utils::colors;
                        if (val < 0.25f) rect->setColor(Black());
                        else if (val < 0.5f) rect->setColor(Blue());
                        else if (val < 0.75f) rect->setColor(Cyan());
                        else rect->setColor(White());
                    }
                }

                app->markDirty(); // Trigger redraw
                genome = survivor; // Evolution
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30 FPS
        }
    });
    worker.detach();

    // -------------------------------------------------
    // Wiring Controls
    // -------------------------------------------------
    btnToggle->onClicked = [simState, btnToggle, app]() {
        bool running = !simState->running;
        simState->running = running;
        btnToggle->setText(running ? "STOP SIMULATION" : "START SIMULATION");

        auto attrs = std::make_shared<job::ansi::Attributes>();
        attrs->setBackground(running ? job::ansi::utils::colors::Red() : job::ansi::utils::colors::Green());
        attrs->setForeground(job::ansi::utils::colors::White());
        btnToggle->setNormalAttributes(attrs);

        app->markDirty();
    };

    return app->exec();
}
