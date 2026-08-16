#pragma once

#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <job_obj_hash.h>

#include "jobsound_export.h"
#include "pipewire_link.h"

namespace job::sound {

class JOBSOUND_EXPORT PipewireLinkLayer
{
public:
    using Ptr  = std::shared_ptr<PipewireLinkLayer>;
    using WPtr = std::weak_ptr<PipewireLinkLayer>;
    using UPtr = std::unique_ptr<PipewireLinkLayer>;

    using LinkModel = core::JobObjHashFast<PipeWireLink::UPtr>;

    struct Point
    {
        float x = 0.0f;
        float y = 0.0f;
    };

    struct Line
    {
        std::vector<Point> points;
    };

    struct Triangle
    {
        Point a;
        Point b;
        Point c;
    };

    struct Geometry
    {
        std::vector<Line> lines;
        std::vector<Triangle> triangles;

        void clear()
        {
            lines.clear();
            triangles.clear();
        }

        [[nodiscard]] bool isEmpty() const noexcept
        {
            return lines.empty() && triangles.empty();
        }
    };

    enum class LineType : std::uint8_t {
        Straight = 0,
        Bezier,
        SchematicArrow,
        TriangleArrow
    };

    static Ptr createShared()
    {
        return std::make_shared<PipewireLinkLayer>();
    }

    static UPtr createUnique()
    {
        return std::make_unique<PipewireLinkLayer>();
    }

    explicit PipewireLinkLayer() = default;

    PipewireLinkLayer(const PipewireLinkLayer &) = default;
    PipewireLinkLayer(PipewireLinkLayer &&) noexcept = default;

    PipewireLinkLayer &operator=(const PipewireLinkLayer &) = default;
    PipewireLinkLayer &operator=(PipewireLinkLayer &&) noexcept = default;

    ~PipewireLinkLayer() = default;

    [[nodiscard]] LineType lineType() const noexcept
    {
        return m_lineType;
    }

    void setLineType(LineType type) noexcept
    {
        m_lineType = type;
    }

    [[nodiscard]] static const std::vector<std::string> &lineTypeNames()
    {
        static const std::vector<std::string> names{
            "Straight",
            "Bezier",
            "SchematicArrow",
            "TriangleArrow"
        };

        return names;
    }

    void setPortPosition(std::uint32_t portId, Point position)
    {
        m_portPositions[portId] = position;
    }

    void removePortPosition(std::uint32_t portId)
    {
        m_portPositions.erase(portId);
    }

    void clearPortPositions() noexcept
    {
        m_portPositions.clear();
    }

    [[nodiscard]] bool hasPortPosition(std::uint32_t portId) const
    {
        return m_portPositions.contains(portId);
    }

    [[nodiscard]] const std::unordered_map<std::uint32_t, Point> &portPositions() const noexcept
    {
        return m_portPositions;
    }

    [[nodiscard]] Geometry build(const LinkModel &links) const
    {
        Geometry geometry;

        for (const auto &item : links) {
            const auto &link = item.second;

            if (!link)
                continue;

            const auto outIt =
                m_portPositions.find(link->outputPortId());

            const auto inIt =
                m_portPositions.find(link->inputPortId());

            if (outIt == m_portPositions.end() || inIt == m_portPositions.end())
                continue;

            const Point p1 = outIt->second;
            const Point p2 = inIt->second;

            switch (m_lineType) {
            case LineType::Straight:
                buildStraight(geometry, p1, p2);
                break;

            case LineType::Bezier:
                buildBezier(geometry, p1, p2);
                break;

            case LineType::SchematicArrow:
                buildSchematicArrow(geometry, p1, p2);
                break;

            case LineType::TriangleArrow:
                buildTriangleArrow(geometry, p1, p2);
                break;
            }
        }

        return geometry;
    }

private:
    static void buildStraight(Geometry &geometry, const Point &p1, const Point &p2)
    {
        Line line;
        line.points.reserve(2);

        line.points.push_back(p1);
        line.points.push_back(p2);

        geometry.lines.push_back(std::move(line));
    }

    static void buildBezier(Geometry &geometry, const Point &p1, const Point &p2)
    {
        constexpr std::size_t segments = 20;

        const Point c1{
            p1.x + 60.0f,
            p1.y
        };

        const Point c2{
            p2.x - 60.0f,
            p2.y
        };

        Line line;
        line.points.reserve(segments);

        for (std::size_t i = 0; i < segments; ++i) {
            const float t = static_cast<float>(i) / static_cast<float>(segments - 1);
            const float u = 1.0f - t;

            Point point;
            point.x =
                u * u * u * p1.x +
                3.0f * u * u * t * c1.x +
                3.0f * u * t * t * c2.x +
                t * t * t * p2.x;

            point.y =
                u * u * u * p1.y +
                3.0f * u * u * t * c1.y +
                3.0f * u * t * t * c2.y +
                t * t * t * p2.y;

            line.points.push_back(point);
        }

        geometry.lines.push_back(std::move(line));
    }

    static void buildSchematicArrow(Geometry &geometry, const Point &p1, const Point &p2)
    {
        buildStraight(geometry, p1, p2);

        const Point dir = normalizedDirection(p1, p2);

        const Point marker{
            p2.x - dir.x * 10.0f,
            p2.y - dir.y * 10.0f
        };

        Line markerLine;
        markerLine.points.reserve(2);

        markerLine.points.push_back(marker);
        markerLine.points.push_back(p2);

        geometry.lines.push_back(std::move(markerLine));
    }

    static void buildTriangleArrow(Geometry &geometry, const Point &p1, const Point &p2)
    {
        buildStraight(geometry, p1, p2);

        const Point dir = normalizedDirection(p1, p2);

        const Point perp{
            -dir.y,
            dir.x
        };

        const Point left{
            p2.x - dir.x * 10.0f + perp.x * 5.0f,
            p2.y - dir.y * 10.0f + perp.y * 5.0f
        };

        const Point right{
            p2.x - dir.x * 10.0f - perp.x * 5.0f,
            p2.y - dir.y * 10.0f - perp.y * 5.0f
        };

        geometry.triangles.push_back({ p2, left, right });
    }

    [[nodiscard]] static Point normalizedDirection(const Point &from, const Point &to) noexcept
    {
        const float dx = to.x - from.x;
        const float dy = to.y - from.y;

        const float length =
            std::hypot(dx, dy);

        if (length <= 0.0f)
            return {};

        return {
            dx / length,
            dy / length
        };
    }

private:
    LineType m_lineType = LineType::Straight;
    std::unordered_map<std::uint32_t, Point> m_portPositions;
};

} // namespace job::sound