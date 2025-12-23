#include <memory>
#include <iostream>
#include <thread>
#include <atomic>
#include <string>

#include <io_factory.h>
#include <job_ansi_attributes.h>
#include <job_tui_core_application.h>
#include <containers/window.h>
#include <elements/rectangle.h>
#include <job_tui_mouse_area.h>

// AI Backend
#include <ctx/job_stealing_ctx.h>
#include <es_coach.h>
#include <learn_type.h>
#include <layer_factory.h> // Needed for LayerGene

using namespace job::io;
using namespace job::ansi;
using namespace job::tui;
using namespace job::tui::gui;

// ---------------------------------------------------------
// HELPER: Create the Neural Network Topology
// ---------------------------------------------------------
job::ai::evo::Genome buildNetwork() {
    using namespace job::ai::evo;
    using namespace job::ai::layers;

    Genome g;
    // Layer 1: 4 Inputs -> 32 Hidden (Tanh is stable)
    LayerGene l1{};
    l1.type = LayerType::Dense;
    l1.activation = comp::ActivationType::Tanh;
    l1.inputs = 4;
    l1.outputs = 32;
    l1.weightCount = (4 * 32) + 32;
    l1.weightOffset = 0;
    g.architecture.push_back(l1);

    // Layer 2: 32 Hidden -> 2 Output (Left/Right)
    LayerGene l2{};
    l2.type = LayerType::Dense;
    l2.activation = comp::ActivationType::Identity;
    l2.inputs = 32;
    l2.outputs = 2;
    l2.weightCount = (32 * 2) + 2;
    l2.weightOffset = l1.weightCount;
    g.architecture.push_back(l2);

    // Initialize Weights (Random Noise)
    g.weights.resize(l1.weightCount + l2.weightCount);
    for(auto& w : g.weights)
        w = ((rand() % 200) / 100.0f - 1.0f); // -1.0 to 1.0

    return g;
}

int main() {
    auto device = IOFactory::createFromType(FactoryType::FILE_STD_OUT, "");
    if (!device || !device->openDevice()) return 1;

    auto app = std::make_shared<JobTuiCoreApplication>(device);
    // Taller window to see the title clearly
    auto root = Window::create({2, 1, 80, 20}, "Neural CartPole: Idle", app.get());

    // 1. LEFT RECTANGLE (Start/Stop)
    auto rect1 = Rectangle::create(root.get());
    rect1->setGeometry(5, 3, 30, 10);
    rect1->setColor("blue");

    // 2. RIGHT RECTANGLE (Status)
    auto rect2 = Rectangle::create(root.get());
    rect2->setGeometry(40, 3, 30, 10);
    rect2->setColor("dark_gray");

    // 3. AI BACKEND
    std::atomic<bool> runSim{false};

    std::thread aiThread([rect2, root, app, &runSim]() {
        job::threads::JobStealerCtx ctx(8);

        job::ai::coach::ESCoach::Config cfg;
        cfg.envConfig.type  = job::ai::learn::LearnType::CartPole;
        cfg.populationSize = 128;
        cfg.sigma = 0.5f; // Good noise level
        cfg.envConfig.initWsMb = 1;

        job::ai::coach::ESCoach coach(ctx.pool, cfg);

        coach.coach(buildNetwork());

        while(true) {
            if (runSim) {
                // Train
                coach.coach(coach.bestGenome());
                float score = coach.currentBestFitness();

                // UI Logic
                std::string status = "Training: " + std::to_string((int)score);

                if (score > 490.0f) {
                    rect2->setColor("yellow"); // Victory!
                    status += " (SOLVED)";
                } else {
                    rect2->setColor("green");  // Learning...
                }

                // Update Title safely (assuming thread safe or risky-but-ok for simple app)
                root->setTitle(status);
                app->markDirty();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    aiThread.detach();

    // 4. MOUSE
    auto mouseOne = JobTuiMouseArea::create(rect1.get());
    mouseOne->onClicked = [rect1, root, app, &runSim](const Event &) {
        bool running = !runSim;
        runSim = running;

        if (running) {
            rect1->setColor("red");
        } else {
            rect1->setColor("blue");
            root->setTitle("Paused");
        }
        app->markDirty();
    };

    return app->exec();
}
