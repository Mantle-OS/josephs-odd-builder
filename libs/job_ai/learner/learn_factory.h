#pragma once

#include <memory>
#include <job_logger.h>

#include "learn_type.h"
#include "ilearn.h"
#include "learn_xor.h"
#include "learn_cart_pole.h"
#include "learn_bard.h"

namespace job::ai::learn {

[[nodiscard]] inline std::unique_ptr<ILearn> makeLearner(LearnType type, threads::ThreadPool::Ptr pool)
{
    switch (type) {
    case LearnType::XOR:
        return XORLearn::create(std::move(pool));
    case LearnType::CartPole:
        return CartPoleLearn::create(std::move(pool));
    case LearnType::Bard:
        return BardLearn::create(std::move(pool));

    default:
        JOB_LOG_ERROR("Unknown Learner Type: {}", static_cast<uint8_t>(type));
        return nullptr;
    }
}

} // namespace job::ai::learn
