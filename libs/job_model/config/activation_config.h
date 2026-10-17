#pragma once

#include <obj/job_obj_class.h>
#include <obj/job_obj_members.h>
#include <string>
#include "job_activation.h"

JBaseObj(ActivationConfig , job::model,
    JRW(job::model::ActivationType, activation, job::model::ActivationType::Unknown),
    JRW(std::string, activationString, "unknown")
)
