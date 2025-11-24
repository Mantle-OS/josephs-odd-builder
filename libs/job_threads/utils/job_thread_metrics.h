#include <vector>
namespace job::threads {
struct JobThreadMetrics {
    static constexpr size_t kDefaultPriorityLevels = 10;
    std::vector<size_t>     byPriority;
    size_t                  total{0};
    double                  loadAvg{0.0};
};

} // namespace job::threads
// CHECKPOINT: v1.2
