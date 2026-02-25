#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <type_traits>
#include <utility>

#include "utils/job_pipeline_stage.h"
#include "utils/job_pipeline_sink.h"

namespace job::threads {

class JobPipeline {
public:
    explicit JobPipeline(ThreadPool::Ptr pool) :
        m_pool(std::move(pool))
    {
        if (!m_pool)
            JOB_LOG_ERROR("[JobPipeline] ThreadPool is null! Not good !");
    }

    template <typename T>
    std::shared_ptr<JobPipelineStage<T, T>> createInput(size_t cap = 1024)
    {
        // forward "everything"
        auto stage = make_pipeline_stage<T, T>(m_pool, [](T &&msg)-> std::optional<T> {return std::move(msg);}, cap);
        keepAlive(stage);
        return stage;
    }

    template <typename In, typename Out>
    std::shared_ptr<JobPipelineStage<In, Out>> createStage(typename JobPipelineStage<In, Out>::ProcessFunc func, size_t cap = 1024)
    {
        auto stage = make_pipeline_stage<In, Out>(m_pool, std::move(func), cap);
        keepAlive(stage);
        return stage;
    }

    // EOL
    template <typename In>
    std::shared_ptr<JobPipelineSink<In>> createSink(typename JobPipelineSink<In>::ConsumeFunc func,  size_t cap = 1024)
    {
        auto sink = make_pipeline_sink<In>(m_pool, std::move(func), cap);
        keepAlive(sink);
        return sink;
    }

    // "linear chain" connect(source, stage1, stage2, sink);
    template <typename Head, typename Next, typename... Rest>
    static void connect(std::shared_ptr<Head> head, std::shared_ptr<Next> next, Rest... rest)
    {
        if (head && next) {
            using HeadOut = typename Head::OutputType;
            using NextIn  = typename Next::InputType;
            static_assert(std::is_same_v<HeadOut, NextIn>,
                          "JobPipeline::connect: incompatible stage types");

            // Head::NextBase -> JobThreadActor<HeadOut>
            using NextBase = typename Head::NextBase;
            std::shared_ptr<NextBase> asBase = std::static_pointer_cast<NextBase>(next);
            head->addNext(std::move(asBase));
        }

        if constexpr (sizeof...(rest) > 0)
            connect(next, rest...);
    }

    template <typename Head, typename Next>
    static void connectFanOut(std::shared_ptr<Head> head, const std::vector<std::shared_ptr<Next>>& nexts)
    {
        if (!head)
            return;

        using HeadOut = typename Head::OutputType;
        using NextIn  = typename Next::InputType;
        static_assert(std::is_same_v<HeadOut, NextIn>, "JobPipeline::connectFanOut: incompatible stage types");

        using NextBase = typename Head::NextBase;
        for (auto &n : nexts) {
            if (n) {
                std::shared_ptr<NextBase> asBase = std::static_pointer_cast<NextBase>(n);
                head->addNext(std::move(asBase));
            }
        }
    }

    void clear()
    {
        m_stages.clear();
    }

private:
    void keepAlive(std::shared_ptr<void> stage)
    {
        m_stages.push_back(std::move(stage));
    }

    ThreadPool::Ptr                    m_pool;
    std::vector<std::shared_ptr<void>> m_stages;
};

} // namespace job::threads

