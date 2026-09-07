#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <job_base_obj.h>

// #include "jobio_export.h" JOBIO_EXPORT
#include "pty_dot.h"
#include "pty_env.h"

namespace job::io {

class  PtyEnviron : public core::BaseObject
{
public:
    using Ptr = std::shared_ptr<PtyEnviron>;
    using WPtr = std::weak_ptr<PtyEnviron>;
    using UPtr = std::unique_ptr<PtyEnviron>;

    using EnvList = std::vector<PtyEnv>;
    using DotList = std::vector<PtyDot>;

    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    PtyEnviron();
    ~PtyEnviron() override = default;

    PtyEnviron(const PtyEnviron &) = default;
    PtyEnviron &operator=(const PtyEnviron &) = default;
    PtyEnviron(PtyEnviron &&) noexcept = default;
    PtyEnviron &operator=(PtyEnviron &&) noexcept = default;

    template <typename... Args>
    [[nodiscard]] static Ptr createShared(Args &&...args)
    {
        return std::make_shared<PtyEnviron>(std::forward<Args>(args)...);
    }

    template <typename... Args>
    [[nodiscard]] static UPtr createUniq(Args &&...args)
    {
        return std::make_unique<PtyEnviron>(std::forward<Args>(args)...);
    }

    // Build the initial environment from the current process environ.
    [[nodiscard]] bool buildInitList();

    // Replace the resolved environment with a newly captured environment.
    void setResolved(EnvList environ);

    // Reset the resolved environment back to the initial environment.
    void resetResolved();

    // Initial environment -----------------------------------------------------

    [[nodiscard]] const EnvList &initial() const noexcept;

    [[nodiscard]] bool containsInitial(std::string_view name) const noexcept;
    [[nodiscard]] std::size_t initialIndexOf(std::string_view name) const noexcept;

    [[nodiscard]] const PtyEnv *initialEnv(std::string_view name) const noexcept;
    [[nodiscard]] PtyEnv *initialEnv(std::string_view name) noexcept;

    // Resolved environment ----------------------------------------------------

    [[nodiscard]] const EnvList &resolved() const noexcept;
    [[nodiscard]] EnvList &resolved() noexcept;

    [[nodiscard]] bool contains(std::string_view name) const noexcept;
    [[nodiscard]] std::size_t indexOf(std::string_view name) const noexcept;

    [[nodiscard]] const PtyEnv *env(std::string_view name) const noexcept;
    [[nodiscard]] PtyEnv *env(std::string_view name) noexcept;

    [[nodiscard]] std::string_view value(std::string_view name,
                                         std::string_view defaultValue = {}) const noexcept;

    // Environment mutation ---------------------------------------------------

    [[nodiscard]] bool add(PtyEnv env);

    [[nodiscard]] bool set(std::string_view name,
                           std::string_view value,
                           bool enabled = true,
                           PtyEnv::EnvType envType = PtyEnv::EnvType::Custom);

    [[nodiscard]] bool setDefault(std::string_view name,
                                  std::string_view value,
                                  bool enabled = true,
                                  PtyEnv::EnvType envType = PtyEnv::EnvType::Custom);

    [[nodiscard]] bool append(std::string_view name,
                              std::string_view value,
                              std::string_view separator = {});

    [[nodiscard]] bool prepend(std::string_view name,
                               std::string_view value,
                               std::string_view separator = {});

    [[nodiscard]] bool remove(std::string_view name);

    void clearResolved() noexcept;

    // Dot/environment sources ------------------------------------------------

    [[nodiscard]] const DotList &dotFiles() const noexcept;
    [[nodiscard]] DotList &dotFiles() noexcept;

    [[nodiscard]] bool addDotFile(PtyDot dot);
    [[nodiscard]] bool removeDotFile(std::string_view path);

    [[nodiscard]] bool containsDotFile(std::string_view path) const noexcept;
    [[nodiscard]] std::size_t dotFileIndexOf(std::string_view path) const noexcept;

    [[nodiscard]] const PtyDot *dotFile(std::string_view path) const noexcept;
    [[nodiscard]] PtyDot *dotFile(std::string_view path) noexcept;

    // Hash every enabled source file and store the resulting hash on PtyDot.
    [[nodiscard]] bool hashDotFiles();

    void clearDotHashes() noexcept;

    // Helpers ----------------------------------------------------------------

    [[nodiscard]] std::vector<std::string> toStrings() const; // << not really needed

    [[nodiscard]] bool isEmpty() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

private:
    EnvList m_initial;
    EnvList m_environ;

    DotList m_dotFiles;
};

} // namespace job::io