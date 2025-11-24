#include <future>
#include <memory>
#include <stop_token>
namespace job::threads {

template <typename T, typename R>
struct ThreadArgs {
    T                                   *self;
    std::shared_ptr<std::promise<R>>    promise;
    std::stop_token                     token;
};

} // namespace job::threads
// CHECKPOINT: v1.5
