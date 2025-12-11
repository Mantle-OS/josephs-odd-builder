#pragma once

namespace job::tui {

struct Rect {
    int x{0};
    int y{0};
    int width{0};
    int height{0};

    Rect() = default;
    Rect(int x_, int y_, int w_, int h_) :
        x(x_),
        y(y_),
        width(w_),
        height(h_)
    {
    }

    int left() const
    {
        return x;
    }
    int top() const
    {
        return y;
    }
    int right() const
    {
        return x + width - 1;
    }
    int bottom() const
    {
        return y + height - 1;
    }

    bool isValid() const
    {
        return width > 0 && height > 0;
    }

    bool contains(int px, int py) const
    {
        return px >= x && px < (x + width) && py >= y && py < (y + height);
    }

    bool contains(const Rect &other) const
    {
        return contains(other.x, other.y) &&
               contains(other.right(), other.bottom());
    }

    bool intersects(const Rect &other) const
    {
        return !(other.right() < x || other.x > right() ||
                 other.bottom() < y || other.y > bottom());
    }

    bool operator==(const Rect &other) const
    {
        return x == other.x && y == other.y &&
               width == other.width && height == other.height;
    }

    bool operator!=(const Rect &other) const
    {
        return !(*this == other);
    }
};

}
