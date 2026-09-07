#include "job_yaml.h"

// TODO(job_yaml):
// job_yaml is temporarily built as a compiled C++26 library because exposing
// its C++26 / reflection / contracts requirements through an INTERFACE target
// currently has too wide a blast radius into downstream Qt targets.
//
// Revisit making this header-only again once the surrounding dependency graph
// can safely consume C++26 usage requirements.
namespace job::yaml {

}