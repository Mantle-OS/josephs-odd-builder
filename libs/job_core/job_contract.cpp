#include "job_contract.h"

namespace job::core {

std::atomic<ContractCallback> ContractState::s_callback{nullptr};

void setContractCallback(ContractCallback callback) noexcept
{
    ContractState::setCallback(callback);
}

void clearContractCallback() noexcept
{
    ContractState::setCallback(nullptr);
}

ContractCallback contractCallback() noexcept
{
    return ContractState::callback();
}

void dispatchContractViolation(const ContractViolation &violation) noexcept
{
    if (const ContractCallback callback = contractCallback()) {
        callback(violation);
        return;
    }

    std::contracts::invoke_default_contract_violation_handler(violation);
}

} // namespace job::core

void handle_contract_violation(const std::contracts::contract_violation &violation)
{
    job::core::dispatchContractViolation(violation);
}