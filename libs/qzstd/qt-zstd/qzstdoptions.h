#pragma once
#include <QObject>
#include <QString>
#include <property-macros.h>
#include <job_zstd_options.h>
class QZstdOptions : public QObject
{
    Q_OBJECT

    QP_RW(QString,  input,                      ""                                                      )
    QP_RW(QString,  output,                     ""                                                      )
    QP_RO(int,      current,                    0                                                       )
    QP_RO(int,      total,                      0                                                       )
    QP_RW(int,      compressionLevel,           job::zstd::JobZstdOptions::kDefaultCompressionLevel     )
    QP_RW(bool,     preserveEmptyDirectories,   true                                                    )
    QP_RW(bool,     preserveSymlinks,           true                                                    )
    QP_RW(bool,     recursiveDirectories,       true                                                    )
    QP_RO(QString,  errorString,                ""                                                      )

public:
    QZstdOptions(QObject *parent = nullptr) :
        QObject{parent}
    {

    }
    QZstdOptions(const job::zstd::JobZstdOptions &other, QObject *parent = nullptr) :
        QObject{parent}
    {
        *this = other;
    }

    ~QZstdOptions() override = default;

    [[nodiscard]] bool operator==(const QZstdOptions &other) const noexcept
    {
        return get_input()                      == other.get_input()
        && get_output()                         == other.get_output()
            && get_current()                    == other.get_current()
            && get_total()                      == other.get_total()
            && get_compressionLevel()           == other.get_compressionLevel()
            && get_preserveEmptyDirectories()   == other.get_preserveEmptyDirectories()
            && get_preserveSymlinks()           == other.get_preserveSymlinks()
            && get_recursiveDirectories()       == other.get_recursiveDirectories()
            && get_errorString()                == other.get_errorString();
    }
    [[nodiscard]] bool operator==(const job::zstd::JobZstdOptions &other) const noexcept
    {
        return get_input()                      == other.input()
        && get_output()                         == other.output()
            && get_current()                    == other.current()
            && get_total()                      == other.total()
            && get_compressionLevel()           == other.compressionLevel()
            && get_preserveEmptyDirectories()   == other.preserveEmptyDirectories()
            && get_preserveSymlinks()           == other.preserveSymlinks()
            && get_recursiveDirectories()       == other.recursiveDirectories()
            && get_errorString()                == other.errorString();
    }

    [[nodiscard]] bool operator!=(const QZstdOptions &other) const noexcept
    {
        return !(*this == other);
    }

    [[nodiscard]] bool operator!=(const job::zstd::JobZstdOptions &other) const noexcept
    {
        return !(*this == other);
    }

    QZstdOptions &operator=(const job::zstd::JobZstdOptions &other)
    {
        set_input(QString::fromStdString(other.input()));
        set_output(QString::fromStdString(other.output()));
        set_current(other.current());
        set_total(other.total());
        set_compressionLevel(other.compressionLevel());
        set_preserveEmptyDirectories(other.preserveEmptyDirectories());
        set_preserveSymlinks(other.preserveSymlinks());
        set_recursiveDirectories(other.recursiveDirectories());
        set_errorString(QString::fromStdString(other.errorString()));
        return *this;
    }

    [[nodiscard]] job::zstd::JobZstdOptions toJobZstdOptions() const
    {
        job::zstd::JobZstdOptions opts;

        opts.setInput(get_input().toStdString());
        opts.setOutput(get_output().toStdString());
        opts.setCompressionLevel(get_compressionLevel());
        opts.setPreserveEmptyDirectories(get_preserveEmptyDirectories());
        opts.setPreserveSymlinks(get_preserveSymlinks());
        opts.setRecursiveDirectories(get_recursiveDirectories());

        return opts;
    }



Q_SIGNALS:
    void finished();
};