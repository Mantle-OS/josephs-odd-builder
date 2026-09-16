#include "job_base_obj.h"

namespace job::core {

BaseObject::BaseObject() = default;
BaseObject::~BaseObject() = default;
BaseObject::BaseObject(const BaseObject &) = default;
BaseObject &BaseObject::operator=(const BaseObject &) = default;
BaseObject::BaseObject(BaseObject &&) noexcept = default;
BaseObject &BaseObject::operator=(BaseObject &&) noexcept = default;

} // namespace job::core