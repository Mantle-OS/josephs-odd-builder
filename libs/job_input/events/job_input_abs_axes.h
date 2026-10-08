#pragma once

#include <cstddef>

#include <linux/input-event-codes.h>

#include "job_input_traits.h"

namespace job::input {

class JobInputAbsoluteAxis
{
public:
    inline static constexpr JobInputCode kAbsoluteAxisMax = ABS_MAX;
    inline static constexpr std::size_t kAbsoluteAxisCount = ABS_CNT;

    constexpr JobInputAbsoluteAxis() noexcept = delete;
    ~JobInputAbsoluteAxis() = delete;

    [[nodiscard]] static constexpr bool valid(JobInputCode code) noexcept
    {
        if (code > kAbsoluteAxisMax)
            return false;

        return x(code)              ||
               y(code)              ||
               z(code)              ||
               rx(code)             ||
               ry(code)             ||
               rz(code)             ||
               throttle(code)       ||
               rudder(code)         ||
               wheel(code)          ||
               gas(code)            ||
               brake(code)          ||
               hat0X(code)          ||
               hat0Y(code)          ||
               hat1X(code)          ||
               hat1Y(code)          ||
               hat2X(code)          ||
               hat2Y(code)          ||
               hat3X(code)          ||
               hat3Y(code)          ||
               pressure(code)       ||
               distance(code)       ||
               tiltX(code)          ||
               tiltY(code)          ||
               toolWidth(code)      ||
               volume(code)         ||
               profile(code)        ||
               soundProfile(code)   ||
               misc(code)           ||
               reserved(code)       ||
               mtSlot(code)         ||
               mtTouchMajor(code)   ||
               mtTouchMinor(code)   ||
               mtWidthMajor(code)   ||
               mtWidthMinor(code)   ||
               mtOrientation(code)  ||
               mtPositionX(code)    ||
               mtPositionY(code)    ||
               mtToolType(code)     ||
               mtBlobId(code)       ||
               mtTrackingId(code)   ||
               mtPressure(code)     ||
               mtDistance(code)     ||
               mtToolX(code)        ||
               mtToolY(code);
    }

    [[nodiscard]] static constexpr bool x(JobInputCode code) noexcept {             return code == ABS_X;               }
    [[nodiscard]] static constexpr bool y(JobInputCode code) noexcept {             return code == ABS_Y;               }
    [[nodiscard]] static constexpr bool z(JobInputCode code) noexcept {             return code == ABS_Z;               }
    [[nodiscard]] static constexpr bool rx(JobInputCode code) noexcept {            return code == ABS_RX;              }
    [[nodiscard]] static constexpr bool ry(JobInputCode code) noexcept {            return code == ABS_RY;              }
    [[nodiscard]] static constexpr bool rz(JobInputCode code) noexcept {            return code == ABS_RZ;              }
    [[nodiscard]] static constexpr bool throttle(JobInputCode code) noexcept {      return code == ABS_THROTTLE;        }
    [[nodiscard]] static constexpr bool rudder(JobInputCode code) noexcept {        return code == ABS_RUDDER;          }
    [[nodiscard]] static constexpr bool wheel(JobInputCode code) noexcept {         return code == ABS_WHEEL;           }
    [[nodiscard]] static constexpr bool gas(JobInputCode code) noexcept {           return code == ABS_GAS;             }
    [[nodiscard]] static constexpr bool brake(JobInputCode code) noexcept {         return code == ABS_BRAKE;           }
    [[nodiscard]] static constexpr bool hat0X(JobInputCode code) noexcept {         return code == ABS_HAT0X;           }
    [[nodiscard]] static constexpr bool hat0Y(JobInputCode code) noexcept {         return code == ABS_HAT0Y;           }
    [[nodiscard]] static constexpr bool hat1X(JobInputCode code) noexcept {         return code == ABS_HAT1X;           }
    [[nodiscard]] static constexpr bool hat1Y(JobInputCode code) noexcept {         return code == ABS_HAT1Y;           }
    [[nodiscard]] static constexpr bool hat2X(JobInputCode code) noexcept {         return code == ABS_HAT2X;           }
    [[nodiscard]] static constexpr bool hat2Y(JobInputCode code) noexcept {         return code == ABS_HAT2Y;           }
    [[nodiscard]] static constexpr bool hat3X(JobInputCode code) noexcept {         return code == ABS_HAT3X;           }
    [[nodiscard]] static constexpr bool hat3Y(JobInputCode code) noexcept {         return code == ABS_HAT3Y;           }
    [[nodiscard]] static constexpr bool pressure(JobInputCode code) noexcept {      return code == ABS_PRESSURE;        }
    [[nodiscard]] static constexpr bool distance(JobInputCode code) noexcept {      return code == ABS_DISTANCE;        }
    [[nodiscard]] static constexpr bool tiltX(JobInputCode code) noexcept {         return code == ABS_TILT_X;          }
    [[nodiscard]] static constexpr bool tiltY(JobInputCode code) noexcept {         return code == ABS_TILT_Y;          }
    [[nodiscard]] static constexpr bool toolWidth(JobInputCode code) noexcept {     return code == ABS_TOOL_WIDTH;      }
    [[nodiscard]] static constexpr bool volume(JobInputCode code) noexcept {        return code == ABS_VOLUME;          }
    [[nodiscard]] static constexpr bool profile(JobInputCode code) noexcept {       return code == ABS_PROFILE;         }
    [[nodiscard]] static constexpr bool soundProfile(JobInputCode code) noexcept {  return code == ABS_SND_PROFILE;     }
    [[nodiscard]] static constexpr bool misc(JobInputCode code) noexcept {          return code == ABS_MISC;            }
    [[nodiscard]] static constexpr bool reserved(JobInputCode code) noexcept {      return code == ABS_RESERVED;        }
    [[nodiscard]] static constexpr bool mtSlot(JobInputCode code) noexcept {        return code == ABS_MT_SLOT;         }
    [[nodiscard]] static constexpr bool mtTouchMajor(JobInputCode code) noexcept {  return code == ABS_MT_TOUCH_MAJOR;  }
    [[nodiscard]] static constexpr bool mtTouchMinor(JobInputCode code) noexcept {  return code == ABS_MT_TOUCH_MINOR;  }
    [[nodiscard]] static constexpr bool mtWidthMajor(JobInputCode code) noexcept {  return code == ABS_MT_WIDTH_MAJOR;  }
    [[nodiscard]] static constexpr bool mtWidthMinor(JobInputCode code) noexcept {  return code == ABS_MT_WIDTH_MINOR;  }
    [[nodiscard]] static constexpr bool mtOrientation(JobInputCode code) noexcept { return code == ABS_MT_ORIENTATION;  }
    [[nodiscard]] static constexpr bool mtPositionX(JobInputCode code) noexcept {   return code == ABS_MT_POSITION_X;   }
    [[nodiscard]] static constexpr bool mtPositionY(JobInputCode code) noexcept {   return code == ABS_MT_POSITION_Y;   }
    [[nodiscard]] static constexpr bool mtToolType(JobInputCode code) noexcept {    return code == ABS_MT_TOOL_TYPE;    }
    [[nodiscard]] static constexpr bool mtBlobId(JobInputCode code) noexcept {      return code == ABS_MT_BLOB_ID;      }
    [[nodiscard]] static constexpr bool mtTrackingId(JobInputCode code) noexcept {  return code == ABS_MT_TRACKING_ID;  }
    [[nodiscard]] static constexpr bool mtPressure(JobInputCode code) noexcept {    return code == ABS_MT_PRESSURE;     }
    [[nodiscard]] static constexpr bool mtDistance(JobInputCode code) noexcept {    return code == ABS_MT_DISTANCE;     }
    [[nodiscard]] static constexpr bool mtToolX(JobInputCode code) noexcept {       return code == ABS_MT_TOOL_X;       }
    [[nodiscard]] static constexpr bool mtToolY(JobInputCode code) noexcept {       return code == ABS_MT_TOOL_Y;       }
};

} // namespace job::input