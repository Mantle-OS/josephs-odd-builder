#include <future>
#include <memory>
namespace job::threads {

template <typename T, typename R>
struct ThreadArgs {
    T* self;
    std::shared_ptr<std::promise<R>> promise;
    std::stop_token token;
};

} // namespace job::thread
// CHECKPOINT: v1.5
