#include "qextrarandom.h"

std::uint64_t QExtraRandom::secureU64()
{
    return job::crypto::JobRandom::secureU64();
}

void QExtraRandom::secureBytes(void *buf, std::size_t size)
{
    job::crypto::JobRandom::secureBytes(buf, size);
}

void QExtraRandom::setGlobalSeed(std::uint64_t seed)
{
    job::crypto::JobRandom::setGlobalSeed(seed);
}

void QExtraRandom::disableGlobalSeed()
{
    job::crypto::JobRandom::disableGlobalSeed();
}

float QExtraRandom::uniformReal(float a, float b)
{
    return job::crypto::JobRandom::uniformReal(a, b);
}

std::uint32_t QExtraRandom::uniformU32(std::uint32_t lo, std::uint32_t hi)
{
    return job::crypto::JobRandom::uniformU32(lo, hi);
}

float QExtraRandom::normal(float mean, float stddev)
{
    return job::crypto::JobRandom::normal(mean, stddev);
}

bool QExtraRandom::bernoulli(float p)
{
    return job::crypto::JobRandom::bernoulli(p);
}

QByteArray QExtraRandom::randomSalt() noexcept
{
    auto const nativeSalt = job::crypto::JobRandom::randomSalt();
    return QByteArray(reinterpret_cast<const char*>(nativeSalt.data()), static_cast<int>(nativeSalt.size()));
}



