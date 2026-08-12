#include "job_ggml_cpu.h"

#include <stdexcept>
#include <utility>

namespace job::ggml {

JobGgmlCpu::JobGgmlCpu(ggml_backend_dev_t device, JobGgmlBackendReg::Ptr backendReg) :
    JobGgmlDevice(device, std::move(backendReg))
{
    if (!isCpuBackend())
        throw std::invalid_argument{ "JobGgmlCpu requires a CPU GGML backend" };
}

bool JobGgmlCpu::isCpuBackend() const noexcept
{
    const JobGgmlBackend::Ptr cpuBackend = backend();

    return cpuBackend &&
           cpuBackend->isValid() &&
           ggml_backend_is_cpu(cpuBackend->backend());
}

void JobGgmlCpu::setNThreads(int nThreads)
{
    const JobGgmlBackend::Ptr cpuBackend = backend();

    if (!cpuBackend || !cpuBackend->isValid())
        throw std::runtime_error{ "Cannot set CPU thread count on an invalid GGML backend" };

    ggml_backend_cpu_set_n_threads(cpuBackend->backend(), nThreads);
}

void JobGgmlCpu::setThreadPool(JobGgmlThreadPool *threadPool)
{
    const JobGgmlBackend::Ptr cpuBackend = backend();

    if (!cpuBackend || !cpuBackend->isValid())
        throw std::runtime_error{ "Cannot set CPU thread pool on an invalid GGML backend" };

    ggml_backend_cpu_set_threadpool(cpuBackend->backend(),
                                    threadPool ? threadPool->threadPool() : nullptr
                                    );
}

void JobGgmlCpu::setAbortCallback(JobGgmlAbortCallback *callback)
{
    const JobGgmlBackend::Ptr cpuBackend = backend();

    if (!cpuBackend || !cpuBackend->isValid())
        throw std::runtime_error{ "Cannot set CPU abort callback on an invalid GGML backend" };

    ggml_backend_cpu_set_abort_callback(cpuBackend->backend(),
                                        callback ? callback->callback() : nullptr,
                                        callback ? callback->callbackData() : nullptr);
}

void JobGgmlCpu::setUseReference(bool useReference)
{
    const JobGgmlBackend::Ptr cpuBackend = backend();
    if (!cpuBackend || !cpuBackend->isValid())
        throw std::runtime_error{ "Cannot set CPU reference mode on an invalid GGML backend" };

    ggml_backend_cpu_set_use_ref(cpuBackend->backend(), useReference);
}

void JobGgmlCpu::initializeNuma(JobGgmlNumaStrategy strategy)
{
    ggml_numa_init(toGgmlNumaStrategy(strategy));
}

bool JobGgmlCpu::isNuma() noexcept
{
    return ggml_is_numa();
}

bool JobGgmlCpu::hasSse3() noexcept
{
    return ggml_cpu_has_sse3() != 0;
}

bool JobGgmlCpu::hasSsse3() noexcept
{
    return ggml_cpu_has_ssse3() != 0;
}

bool JobGgmlCpu::hasAvx() noexcept
{
    return ggml_cpu_has_avx() != 0;
}

bool JobGgmlCpu::hasAvxVnni() noexcept
{
    return ggml_cpu_has_avx_vnni() != 0;
}

bool JobGgmlCpu::hasAvx2() noexcept
{
    return ggml_cpu_has_avx2() != 0;
}

bool JobGgmlCpu::hasBmi2() noexcept
{
    return ggml_cpu_has_bmi2() != 0;
}

bool JobGgmlCpu::hasF16c() noexcept
{
    return ggml_cpu_has_f16c() != 0;
}

bool JobGgmlCpu::hasFma() noexcept
{
    return ggml_cpu_has_fma() != 0;
}

bool JobGgmlCpu::hasAvx512() noexcept
{
    return ggml_cpu_has_avx512() != 0;
}

bool JobGgmlCpu::hasAvx512Vbmi() noexcept
{
    return ggml_cpu_has_avx512_vbmi() != 0;
}

bool JobGgmlCpu::hasAvx512Vnni() noexcept
{
    return ggml_cpu_has_avx512_vnni() != 0;
}

bool JobGgmlCpu::hasAvx512Bf16() noexcept
{
    return ggml_cpu_has_avx512_bf16() != 0;
}

bool JobGgmlCpu::hasAmxInt8() noexcept
{
    return ggml_cpu_has_amx_int8() != 0;
}

bool JobGgmlCpu::hasNeon() noexcept
{
    return ggml_cpu_has_neon() != 0;
}

bool JobGgmlCpu::hasArmFma() noexcept
{
    return ggml_cpu_has_arm_fma() != 0;
}

bool JobGgmlCpu::hasFp16Va() noexcept
{
    return ggml_cpu_has_fp16_va() != 0;
}

bool JobGgmlCpu::hasDotProd() noexcept
{
    return ggml_cpu_has_dotprod() != 0;
}

bool JobGgmlCpu::hasMatMulInt8() noexcept
{
    return ggml_cpu_has_matmul_int8() != 0;
}

bool JobGgmlCpu::hasSve() noexcept
{
    return ggml_cpu_has_sve() != 0;
}

int JobGgmlCpu::sveCount() noexcept
{
    return ggml_cpu_get_sve_cnt();
}

bool JobGgmlCpu::hasSme() noexcept
{
    return ggml_cpu_has_sme() != 0;
}

bool JobGgmlCpu::hasRiscvV() noexcept
{
    return ggml_cpu_has_riscv_v() != 0;
}

int JobGgmlCpu::rvvVectorLength() noexcept
{
    return ggml_cpu_get_rvv_vlen();
}

bool JobGgmlCpu::hasVsx() noexcept
{
    return ggml_cpu_has_vsx() != 0;
}

bool JobGgmlCpu::hasVxe() noexcept
{
    return ggml_cpu_has_vxe() != 0;
}

bool JobGgmlCpu::hasWasmSimd() noexcept
{
    return ggml_cpu_has_wasm_simd() != 0;
}

bool JobGgmlCpu::hasLlamaFile() noexcept
{
    return ggml_cpu_has_llamafile() != 0;
}

std::string JobGgmlCpu::dump()
{
    std::ostringstream stream;

    stream
        << "CPU{"
        << "numa=" << (isNuma() ? "true" : "false")

        << ", x86={"
        << "sse3=" << (hasSse3() ? "true" : "false")
        << ", ssse3=" << (hasSsse3() ? "true" : "false")
        << ", avx=" << (hasAvx() ? "true" : "false")
        << ", avxVnni=" << (hasAvxVnni() ? "true" : "false")
        << ", avx2=" << (hasAvx2() ? "true" : "false")
        << ", bmi2=" << (hasBmi2() ? "true" : "false")
        << ", f16c=" << (hasF16c() ? "true" : "false")
        << ", fma=" << (hasFma() ? "true" : "false")
        << ", avx512=" << (hasAvx512() ? "true" : "false")
        << ", avx512Vbmi=" << (hasAvx512Vbmi() ? "true" : "false")
        << ", avx512Vnni=" << (hasAvx512Vnni() ? "true" : "false")
        << ", avx512Bf16=" << (hasAvx512Bf16() ? "true" : "false")
        << ", amxInt8=" << (hasAmxInt8() ? "true" : "false")
        << '}'

        << ", arm={"
        << "neon=" << (hasNeon() ? "true" : "false")
        << ", fma=" << (hasArmFma() ? "true" : "false")
        << ", fp16Va=" << (hasFp16Va() ? "true" : "false")
        << ", dotProd=" << (hasDotProd() ? "true" : "false")
        << ", matMulInt8=" << (hasMatMulInt8() ? "true" : "false")
        << ", sve=" << (hasSve() ? "true" : "false")
        << ", sveCount=" << sveCount()
        << ", sme=" << (hasSme() ? "true" : "false")
        << '}'

        << ", riscv={"
        << "v=" << (hasRiscvV() ? "true" : "false")
        << ", vectorLength=" << rvvVectorLength()
        << '}'

        << ", other={"
        << "vsx=" << (hasVsx() ? "true" : "false")
        << ", vxe=" << (hasVxe() ? "true" : "false")
        << ", wasmSimd=" << (hasWasmSimd() ? "true" : "false")
        << ", llamaFile=" << (hasLlamaFile() ? "true" : "false")
        << '}'

        << '}';

    return stream.str();
}

} // namespace job::ggml