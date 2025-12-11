#pragma once
#include <memory>
namespace job::tui {
class LayoutEngine {
public:
    using Ptr = std::shared_ptr<LayoutEngine>;
    enum class Orientation {
        Horizontal,
        Vertical
    };
    virtual ~LayoutEngine() = default;
    virtual void applyLayout() = 0;
};

}
