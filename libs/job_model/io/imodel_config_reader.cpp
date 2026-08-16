#include "imodel_config_reader.h"

#include <job_logger.h>

namespace job::model {

bool IModelConfigReader::finalizeAndValidate(const ModelConfig &config, std::string_view readerTag)
{
    if (!config.isValid()) {
        JOB_LOG_ERROR("[{}] Loaded configuration failed validation checks for model '{}'",
                      readerTag, config.archConfig().modelName());
        return false;
    }

    return true;
}

} // namespace job::model
