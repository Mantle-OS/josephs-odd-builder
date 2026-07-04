#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unistd.h>

#include "job_permissions.h"

namespace job::core {

class IODevice {

public:
    using Ptr                   = std::shared_ptr<IODevice>;
    using ReadCallback          = std::function<void(const char *data, size_t size)>;
    using PermissionsCallback   = std::function<void(IOPermissions perms)>;

    virtual ~IODevice() = default;

    [[nodiscard]] virtual bool openDevice() = 0;
    virtual void closeDevice() = 0;

    [[nodiscard]] virtual ssize_t read(char *buffer, size_t maxlen) = 0;
    [[nodiscard]] virtual ssize_t write(const char *data, size_t len) = 0;

    ssize_t write(const std::string &data)
    {
        return write(data.data(), data.size());
    }

    [[nodiscard]] virtual bool isOpen() const = 0;
    [[nodiscard]] virtual int fd() const = 0;

    virtual void setNonBlocking(bool enabled) = 0;

    // Async callback registration not needed for shared memory
    virtual void setReadCallback(ReadCallback cb) = 0;

    virtual bool flush(){ return true;}

    [[nodiscard]] virtual IOPermissions permissions() const
    {
        return m_permissions;
    }

    virtual void setPermissions(IOPermissions perms)
    {
        m_permissions = perms;
        if (m_permissionsCallback) {
            (*m_permissionsCallback)(m_permissions);
        }

    }

    [[nodiscard]] virtual PermissionsCallback *permissionsCallback() const
    {
        if (m_permissionsCallback)
            return m_permissionsCallback;
        return nullptr;
    }

    virtual void setPermissionsCallback(PermissionsCallback *cb)
    {
        if(cb)
            m_permissionsCallback = std::move(cb);
    }

protected:
    [[nodiscard]] ssize_t readToString(std::string &output, size_t maxlen)
    {
        output.resize(maxlen);
        ssize_t bytesRead = read(output.data(), maxlen);
        if (bytesRead < 0) {
            output.clear();
            return -1;
        }
        output.resize(bytesRead);
        return bytesRead;
    }

    [[nodiscard]] ssize_t readToVector(std::vector<uint8_t> &output, size_t maxlen)
    {
        output.resize(maxlen);
        ssize_t bytesRead = read(reinterpret_cast<char*>(output.data()), maxlen);
        if (bytesRead < 0) {
            output.clear();
            return -1;
        }
        output.resize(bytesRead);
        return bytesRead;
    }

    IOPermissions       m_permissions{IOPermissions::DefaultFile};
    PermissionsCallback *m_permissionsCallback{};
};

}
