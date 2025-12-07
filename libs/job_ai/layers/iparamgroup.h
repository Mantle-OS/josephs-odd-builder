#pragma once
#include <vector>
#include "paramgroup_config.h"
#include "paramgroup_type.h"
namespace job::ai::layers {
using ParamGroup = std::vector<ParamGroupConfig>;

class IParamGroup {
public:
    virtual ~IParamGroup() = default;
    virtual ParamGroup paramGroups() = 0;
};

}
