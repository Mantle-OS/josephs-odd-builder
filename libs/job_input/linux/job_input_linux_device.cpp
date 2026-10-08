#include "job_input_linux_device.h"

#include <array>
#include <climits>
#include <fcntl.h>

#include <linux/input.h>

#include <sys/ioctl.h>
#include <unistd.h>

#include <utility>

#include <job_logger.h>

namespace job::input {

JobInputLinuxDevice::JobInputLinuxDevice(fs::path path) noexcept :
    m_path(std::move(path))
{
}

JobInputLinuxDevice::~JobInputLinuxDevice()
{
    closeDevice();
}

JobInputLinuxDevice::JobInputLinuxDevice(JobInputLinuxDevice &&other) noexcept :
    m_fd(std::exchange(other.m_fd, -1)),
    m_path(std::move(other.m_path)),
    m_readCallback(std::move(other.m_readCallback)),
    m_device(std::move(other.m_device))
{
    other.m_device.reset();
}

JobInputLinuxDevice &JobInputLinuxDevice::operator=(JobInputLinuxDevice &&other) noexcept
{
    if (this == &other)
        return *this;

    closeDevice();

    m_fd = std::exchange(other.m_fd, -1);
    m_path = std::move(other.m_path);
    m_readCallback = std::move(other.m_readCallback);
    m_device = std::move(other.m_device);

    other.m_device.reset();

    return *this;
}

bool JobInputLinuxDevice::openDevice()
{
    if (isOpen())
        return true;

    m_fd = ::open(m_path.c_str(), O_RDONLY | O_CLOEXEC);
    if (m_fd < 0)
        return false;

    if (!interrogate()) {
        closeDevice();
        return false;
    }

    return true;
}

void JobInputLinuxDevice::closeDevice()
{
    if (isOpen()) {
        ::close(m_fd);
        m_fd = -1;
    }

    m_device.reset();
}

ssize_t JobInputLinuxDevice::read(char *buffer, size_t maxlen)
{
    if (!isOpen() || !buffer || maxlen == 0)
        return -1;

    return ::read(m_fd, buffer, maxlen);
}

ssize_t JobInputLinuxDevice::write(const char *data, size_t len)
{
    if (!isOpen() || !data || len == 0)
        return -1;

    return ::write(m_fd, data, len);
}

bool JobInputLinuxDevice::isOpen() const
{
    return m_fd >= 0;
}

int JobInputLinuxDevice::fd() const
{
    return m_fd;
}

void JobInputLinuxDevice::setNonBlocking(bool enabled)
{
    if (!isOpen())
        return;

    const int flags = ::fcntl(m_fd, F_GETFL);
    if (flags < 0)
        return;

    const int updatedFlags = enabled ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);

    if (updatedFlags == flags)
        return;

    ::fcntl(m_fd, F_SETFL, updatedFlags);
}

void JobInputLinuxDevice::setReadCallback(ReadCallback cb)
{
    m_readCallback = std::move(cb);
}

bool JobInputLinuxDevice::interrogate()
{
    if (!isOpen())
        return false;

    std::string name;
    std::string physicalLocation;
    std::string uniqueId;

    std::uint16_t busType{};
    std::uint16_t vendor{};
    std::uint16_t product{};
    std::uint16_t version{};

    JobInputCapability capability;

    // Descriptive metadata is optional. Failure simply leaves it empty.
    if (!queryName(name))
        JOB_LOG_DEBUG("[JobInputLinuxDevice] Failed to query name for {}.", pathStr());

    if (!queryPhysicalLocation(physicalLocation))
        JOB_LOG_DEBUG("[JobInputLinuxDevice] Failed to query physical location for {}.", pathStr());

    if (!queryUniqueId(uniqueId))
        JOB_LOG_DEBUG("[JobInputLinuxDevice] Failed to query unique id for {}.", pathStr());

    if (!queryIdentity(busType, vendor, product, version))
        return false;

    if (!queryCapabilities(capability))
        return false;

    m_device.emplace(
        std::move(name),
        std::move(physicalLocation),
        std::move(uniqueId),
        busType,
        vendor,
        product,
        version,
        std::move(capability));

    return true;
}
bool JobInputLinuxDevice::queryName(std::string &name) const
{
    if (!isOpen())
        return false;

    std::array<char, 256> buffer{};

    if (::ioctl(m_fd, EVIOCGNAME(buffer.size()), buffer.data()) < 0)
        return false;

    name = buffer.data();
    return true;
}

bool JobInputLinuxDevice::queryPhysicalLocation(std::string &physicalLocation) const
{
    if (!isOpen())
        return false;

    std::array<char, 256> buffer{};

    if (::ioctl(m_fd, EVIOCGPHYS(buffer.size()), buffer.data()) < 0)
        return false;

    physicalLocation = buffer.data();
    return true;
}

bool JobInputLinuxDevice::queryUniqueId(std::string &uniqueId) const
{
    if (!isOpen())
        return false;

    std::array<char, 256> buffer{};

    if (::ioctl(m_fd, EVIOCGUNIQ(buffer.size()), buffer.data()) < 0)
        return false;

    uniqueId = buffer.data();
    return true;
}

bool JobInputLinuxDevice::queryIdentity(std::uint16_t &busType,
                                        std::uint16_t &vendor,
                                        std::uint16_t &product,
                                        std::uint16_t &version) const
{
    if (!isOpen())
        return false;

    input_id id{};

    if (::ioctl(m_fd, EVIOCGID, &id) < 0)
        return false;

    busType = id.bustype;
    vendor = id.vendor;
    product = id.product;
    version = id.version;

    return true;
}

bool JobInputLinuxDevice::queryCapabilities(JobInputCapability &capability) const
{
    if (!isOpen())
        return false;

    constexpr std::size_t kBitsPerLong = sizeof(unsigned long) * CHAR_BIT;
    constexpr std::size_t kMaximumBitCount = KEY_CNT;
    constexpr std::size_t kMaximumLongCount =
        (kMaximumBitCount + kBitsPerLong - 1) / kBitsPerLong;

    std::array<unsigned long, kMaximumLongCount> bitmap{};

    auto queryBitmap = [this, &bitmap]<typename Bitset>(unsigned long request, Bitset &bits) -> bool {
        bitmap.fill(0);

        if (::ioctl(m_fd, request, bitmap.data()) < 0)
            return false;

        for (std::size_t bit = 0; bit < bits.size(); ++bit) {
            const std::size_t word = bit / kBitsPerLong;
            const std::size_t offset = bit % kBitsPerLong;

            if ((bitmap[word] & (1UL << offset)) != 0)
                bits.set(bit);
        }

        return true;
    };

    auto bitmapBytes = []<typename Bitset>(const Bitset &bits) constexpr -> std::size_t {
        const std::size_t words = (bits.size() + kBitsPerLong - 1) / kBitsPerLong;
        return words * sizeof(unsigned long);
    };

    JobInputCapability::DeviceProperties deviceProperties;
    JobInputCapability::EventTypes eventTypes;
    JobInputCapability::Keys keys;
    JobInputCapability::RelativeAxes relativeAxes;
    JobInputCapability::AbsoluteAxes absoluteAxes;
    JobInputCapability::Switches switches;
    JobInputCapability::MiscEvents miscEvents;
    JobInputCapability::Leds leds;
    JobInputCapability::Sounds sounds;
    JobInputCapability::ForceFeedback forceFeedback;

    if (!queryBitmap(EVIOCGPROP(bitmapBytes(deviceProperties)), deviceProperties))
        return false;

    if (!queryBitmap(EVIOCGBIT(0, bitmapBytes(eventTypes)), eventTypes))
        return false;

    if (eventTypes.test(EV_KEY) &&
        !queryBitmap(EVIOCGBIT(EV_KEY, bitmapBytes(keys)), keys))
        return false;

    if (eventTypes.test(EV_REL) &&
        !queryBitmap(EVIOCGBIT(EV_REL, bitmapBytes(relativeAxes)), relativeAxes))
        return false;

    if (eventTypes.test(EV_ABS) &&
        !queryBitmap(EVIOCGBIT(EV_ABS, bitmapBytes(absoluteAxes)), absoluteAxes))
        return false;

    if (eventTypes.test(EV_SW) &&
        !queryBitmap(EVIOCGBIT(EV_SW, bitmapBytes(switches)), switches))
        return false;

    if (eventTypes.test(EV_MSC) &&
        !queryBitmap(EVIOCGBIT(EV_MSC, bitmapBytes(miscEvents)), miscEvents))
        return false;

    if (eventTypes.test(EV_LED) &&
        !queryBitmap(EVIOCGBIT(EV_LED, bitmapBytes(leds)), leds))
        return false;

    if (eventTypes.test(EV_SND) &&
        !queryBitmap(EVIOCGBIT(EV_SND, bitmapBytes(sounds)), sounds))
        return false;

    if (eventTypes.test(EV_FF) &&
        !queryBitmap(EVIOCGBIT(EV_FF, bitmapBytes(forceFeedback)), forceFeedback))
        return false;

    capability = JobInputCapability(
        std::move(deviceProperties),
        std::move(eventTypes),
        std::move(keys),
        std::move(relativeAxes),
        std::move(absoluteAxes),
        std::move(switches),
        std::move(miscEvents),
        std::move(leds),
        std::move(sounds),
        std::move(forceFeedback));

    return true;
}

} // namespace job::input