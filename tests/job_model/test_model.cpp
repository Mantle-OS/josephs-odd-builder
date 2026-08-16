#include <catch2/catch_test_macros.hpp>
#include "job_model.h"

namespace job::model::test {

TEST_CASE("JobModel initializes in unloaded state and handles missing files gracefully", "[model][facade]")
{
    auto model = JobModel::createUniq();
    REQUIRE(model != nullptr);

    CHECK_FALSE(model->isLoaded());

    // Trying to load a non-existent GGUF file should fail safely without crashing
    bool ok = model->load("/non/existent/path/model.gguf");
    CHECK_FALSE(ok);
    CHECK_FALSE(model->isLoaded());

    // Attempting generation on an unloaded model should return an empty token vector
    std::vector<int32_t> prompt = {1, 2, 3, 4};
    auto tokens = model->generate(prompt, 10);
    CHECK(tokens.empty());

    model->reset();
    CHECK_FALSE(model->isLoaded());
}

} // namespace job::model::test