#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <future>
#include <string>

#include <job_logger.h>
#include <job_thread_pool.h>
#include <sched/job_fifo_scheduler.h>

#include <job_thread_actor.h>

using namespace job::threads;
using namespace std::chrono_literals;

class EchoActor : public JobThreadActor<std::string> {
public:
    using JobThreadActor::JobThreadActor;

    std::future<std::string> getNextMessage()
    {
        m_promise = std::make_shared<std::promise<std::string>>();
        return m_promise->get_future();
    }

protected:
    void onMessage(std::string &&msg) override
    {
        if (m_promise) {
            m_promise->set_value(msg);
            m_promise.reset();
        }
    }

private:
    std::shared_ptr<std::promise<std::string>> m_promise;
};

class CounterActor : public JobThreadActor<int> {
public:
    using JobThreadActor::JobThreadActor;
    std::atomic<int> counter{0};

protected:
    void onMessage(int &&val) override
    {
        counter.fetch_add(val, std::memory_order_relaxed);
    }
};

TEST_CASE("Actor processes basic messages", "[threading][actor][basic]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool = ThreadPool::create(sched, 1);

    // Create Actor
    auto actor = std::make_shared<EchoActor>(pool);

    auto future = actor->getNextMessage();

    actor->post("Hello World");

    REQUIRE(future.wait_for(100ms) == std::future_status::ready);
    REQUIRE(future.get() == "Hello World");

    pool->shutdown();
}

TEST_CASE("Actor handles high volume (Testing Batching)", "[threading][actor][load]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool = ThreadPool::create(sched, 4); // 4 threads

    auto actor = std::make_shared<CounterActor>(pool);

    constexpr int kTotalMessages = 10000;

    for (int i = 0; i < kTotalMessages; ++i)
        actor->post(1);

    int retries = 0;
    while (actor->counter.load() < kTotalMessages && retries < 100) {
        std::this_thread::sleep_for(10ms);
        retries++;
    }

    REQUIRE(actor->counter.load() == kTotalMessages);

    pool->shutdown();
}

TEST_CASE("Actor stays alive to process pending messages", "[threading][actor][lifecycle]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool = ThreadPool::create(sched, 1);
    auto message_processed = std::make_shared<std::atomic<bool>>(false);

    {
        class LifecycleActor : public JobThreadActor<int> {
        public:
            LifecycleActor(ThreadPool::Ptr pool, std::shared_ptr<std::atomic<bool>> flag)
                : JobThreadActor(std::move(pool))
                , m_flag(std::move(flag))
            {
            }

        protected:
            void onMessage(int &&) override
            {
                std::this_thread::sleep_for(10ms);
                m_flag->store(true);
            }

        private:
            std::shared_ptr<std::atomic<bool>> m_flag;
        };

        auto actor = std::make_shared<LifecycleActor>(pool, message_processed);
        actor->post(1);
    }

    int retries = 0;
    while (!message_processed->load() && retries < 100) {
        std::this_thread::sleep_for(10ms);
        retries++;
    }

    REQUIRE(message_processed->load() == true);

    pool->shutdown();
}

TEST_CASE("Actor yields thread after batch limit", "[threading][actor][cooperative]")
{
    auto sched = std::make_shared<FifoScheduler>();
    auto pool = ThreadPool::create(sched, 1);

    auto busyActor = std::make_shared<CounterActor>(pool);
    std::atomic<bool> otherTaskRan{false};

    for(int i=0; i<50; ++i)
        busyActor->post(1);

    pool->submit([&]() {
        otherTaskRan.store(true);
    });

    int retries = 0;
    while (!otherTaskRan.load() && retries < 100) {
        std::this_thread::sleep_for(1ms);
        retries++;
    }

    REQUIRE(otherTaskRan.load() == true);

    while (busyActor->counter.load() < 50)
        std::this_thread::sleep_for(1ms);

    REQUIRE(busyActor->counter.load() == 50);

    pool->shutdown();
}

// CHECKPOINT: v1.0
