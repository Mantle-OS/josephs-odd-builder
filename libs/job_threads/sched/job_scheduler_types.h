#pragma once
namespace job::threads {
enum class SchedulerType {
    Other = 0,
    Fifo,
    RoundRobin,
    WorkStealing,
    Sporadic
};
} //namespace job::threads
