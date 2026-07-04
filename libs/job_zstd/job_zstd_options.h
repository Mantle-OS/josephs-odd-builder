#pragma once

#include <string>
#include <functional>

namespace job::zstd {
class JobZstdOptions
{
public:
    static constexpr int kDefaultCompressionLevel = 3;
    using FinishedCallback = std::function<void()>;

    JobZstdOptions() = default;
    virtual ~JobZstdOptions() = default;

    static const std::string &magicDirString();
    static const std::string &magicEmptyDirString();
    static const std::string &magicFileString();
    static const std::string &magicLinkString();

    [[nodiscard]] static int minCompressionLevel() noexcept;
    [[nodiscard]] static int maxCompressionLevel() noexcept;

    static constexpr int qtDataStreamVersion() noexcept { return 21; }

    [[nodiscard]] const std::string &input() const noexcept;
    bool setInput(const std::string &newInput);

    [[nodiscard]] const std::string &output() const noexcept;
    bool setOutput(const std::string &newOutput);

    [[nodiscard]] int current() const noexcept;
    bool setCurrent(int newCurrent);

    [[nodiscard]] int total() const noexcept;
    bool setTotal(int newTotal);

    [[nodiscard]] int compressionLevel() const noexcept;
    bool setCompressionLevel(int newCompressionLevel);

    [[nodiscard]] const std::string &errorString() const noexcept;
    bool setErrorString(const std::string &newErrorString);

    [[nodiscard]] bool recursiveDirectories() const noexcept;
    bool setRecursiveDirectories(bool value);

    [[nodiscard]] bool preserveEmptyDirectories() const noexcept;
    bool setPreserveEmptyDirectories(bool value);

    [[nodiscard]] bool preserveSymlinks() const noexcept;
    bool setPreserveSymlinks(bool value);

    // Null callback? Nobody's listening, no harm done. Set one and the
    // pipeline actually has something to say when it's done saying it.
    void setOnFinished(FinishedCallback callback);

protected:
    void notifyFinished() const;

private:
    std::string         m_input;
    std::string         m_output;
    int                 m_current                   = 0;
    int                 m_total                     = 0;
    int                 m_compressionLevel          = kDefaultCompressionLevel;
    bool                m_preserveEmptyDirectories  = true;
    bool                m_preserveSymlinks          = true;
    bool                m_recursiveDirectories      = true;
    std::string         m_errorString;
    FinishedCallback    m_onFinished;
};

} // namespace job::zstd