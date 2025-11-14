#pragma once

#include <vector>
#include <functional>
#include <string>
#include <atomic>

namespace job::threads {
struct JobThreadNode {
    std::string                 name;
    std::function<void()>       task;
    std::atomic<int>            depsLeft;
    std::vector<std::string>    depsList;

    explicit JobThreadNode(std::string n, std::function<void()> t):
        name(std::move(n)),
        task(std::move(t)),
        depsLeft(0)
    {}
};
}
