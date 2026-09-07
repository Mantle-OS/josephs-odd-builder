#pragma once

#include <atomic>
#include <contracts>
#include <cstdlib>

#include "jobcore_export.h"

namespace job::core {

using ContractViolation = std::contracts::contract_violation;
using ContractCallback  = void (*)(const ContractViolation &) noexcept;

JOBCORE_EXPORT void setContractCallback(ContractCallback callback) noexcept;
JOBCORE_EXPORT void clearContractCallback() noexcept;
[[nodiscard]] JOBCORE_EXPORT ContractCallback contractCallback() noexcept;

JOBCORE_EXPORT void dispatchContractViolation(const ContractViolation &violation) noexcept;

class JOBCORE_EXPORT ContractState
{
public:
    ContractState() = delete;
    ~ContractState() = delete;

    ContractState(const ContractState &) = delete;
    ContractState &operator=(const ContractState &) = delete;
    ContractState(ContractState &&) = delete;
    ContractState &operator=(ContractState &&) = delete;

    static void setCallback(ContractCallback callback) noexcept
    {
        s_callback.store(callback, std::memory_order_release);
    }

    [[nodiscard]] static ContractCallback callback() noexcept
    {
        return s_callback.load(std::memory_order_acquire);
    }

private:
    static std::atomic<ContractCallback> s_callback;
};

} // namespace job::core

JOBCORE_EXPORT void handle_contract_violation(const std::contracts::contract_violation &violation);

#ifndef JOB_ASSUME
#if defined(__GNUC__) || defined(__clang__)
#define JOB_ASSUME(cond) do { if (!(cond)) __builtin_unreachable(); } while (0)
#else
#define JOB_ASSUME(cond) do { (void)sizeof(cond); } while (0)
#endif
#endif

#ifndef JOB_UNREACHABLE
#if defined(__GNUC__) || defined(__clang__)
#define JOB_UNREACHABLE() do { __builtin_unreachable(); } while (0)
#else
#define JOB_UNREACHABLE() do { std::abort(); } while (0)
#endif
#endif