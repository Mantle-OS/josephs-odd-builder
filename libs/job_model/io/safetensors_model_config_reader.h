#pragma once

#include <memory>
#include <string>
#include <vector>

#include "imodel_config_reader.h"
#include "jobmodel_export.h"

namespace job::model {

class JOBMODEL_EXPORT SafeTensorsModelConfigReader final : public IModelConfigReader
{
public:
    using Ptr  = std::shared_ptr<SafeTensorsModelConfigReader>;
    using WPtr = std::weak_ptr<SafeTensorsModelConfigReader>;
    using UPtr = std::unique_ptr<SafeTensorsModelConfigReader>;

    SafeTensorsModelConfigReader() = default;
    ~SafeTensorsModelConfigReader() override = default;

    [[nodiscard]] static Ptr createShared() { return std::make_shared<SafeTensorsModelConfigReader>(); }
    [[nodiscard]] static UPtr createUniq()  { return std::make_unique<SafeTensorsModelConfigReader>(); }

    SafeTensorsModelConfigReader(const SafeTensorsModelConfigReader &) = default;
    SafeTensorsModelConfigReader &operator=(const SafeTensorsModelConfigReader &) = default;
    SafeTensorsModelConfigReader(SafeTensorsModelConfigReader &&) noexcept = default;
    SafeTensorsModelConfigReader &operator=(SafeTensorsModelConfigReader &&) noexcept = default;

    [[nodiscard]] bool read(const std::filesystem::path &modelPath, ModelConfig &config) override;

private:
    [[nodiscard]] static std::vector<std::string> readTensorNames(const std::filesystem::path &path);
    [[nodiscard]] static std::vector<std::string> readIndexTensorNames(const std::filesystem::path &path);
    [[nodiscard]] static uint32_t estimateBlockCount(const std::vector<std::string> &tensorNames);
};

} // namespace job::model