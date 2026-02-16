#pragma once
#include <algorithm>

#include "job_ansi_enums.h"
#include "job_ansi_screen.h"
#include "esc/csi/dispatch_base.h"


namespace job::ansi::csi {
class DispatchRectOps : public DispatchBase<CSI_CODE> {
public:
    explicit DispatchRectOps(Screen *screen) :
        DispatchBase<CSI_CODE>(screen)
    {
        initMap();
    }

    void initMap() override
    {
        // DECFRA - Fill Rectangular Area
        m_dispatchMap[CSI_CODE::DECFRA] = [this](std::span<const int> params) {
            // Default values per DEC spec
            int value = params.size() > 0 ? params[0] : 0;      // Character to fill
            int top = params.size() > 1 ? params[1] - 1 : 0;    // Top row (1-based)
            int left = params.size() > 2 ? params[2] - 1 : 0;   // Left column (1-based)
            int bottom = params.size() > 3 ? params[3] - 1      // Bottom row (1-based)
                        : m_screen->rowCount() - 1;
            int right = params.size() > 4 ? params[4] - 1       // Right column (1-based)
                       : m_screen->columnCount() - 1;

            // Validate and clamp coordinates
            top = std::clamp(top, 0, m_screen->rowCount() - 1);
            left = std::clamp(left, 0, m_screen->columnCount() - 1);
            bottom = std::clamp(bottom, 0, m_screen->rowCount() - 1);
            right = std::clamp(right, 0, m_screen->columnCount() - 1);

            // Ensure proper ordering
            if (top > bottom) std::swap(top, bottom);
            if (left > right) std::swap(left, right);

            // Fill the rectangular area with the specified character
            for (int row = top; row <= bottom; ++row) {
                for (int col = left; col <= right; ++col) {
                    if (auto *cell = m_screen->cellAt(row, col)) {
                        cell->setChar(static_cast<char32_t>(value));
                        m_screen->markCellDirty(row, col);
                    }
                }
            }
            m_screen->requestRender();
        };

        // DECCRA - Copy Rectangular Area
        m_dispatchMap[CSI_CODE::DECCRA] = [this](std::span<const int> params) {
            // Source rectangle coordinates (1-based)
            int srcTop    = params.size() > 0 ? params[0] - 1 : 0;
            int srcLeft   = params.size() > 1 ? params[1] - 1 : 0;
            int srcBottom = params.size() > 2 ? params[2] - 1 : m_screen->rowCount() - 1;
            int srcRight  = params.size() > 3 ? params[3] - 1 : m_screen->columnCount() - 1;

            // Source and destination pages (ignored in our implementation)
            // int srcPage = params.size() > 4 ? params[4] : 1;

            // Destination coordinates (1-based)
            int dstTop = params.size() > 5 ? params[5] - 1 : 0;
            int dstLeft = params.size() > 6 ? params[6] - 1 : 0;
            // int dstPage = params.size() > 7 ? params[7] : 1;

            // Validate and clamp coordinates
            srcTop    = std::clamp(srcTop, 0, m_screen->rowCount() - 1);
            srcLeft   = std::clamp(srcLeft, 0, m_screen->columnCount() - 1);
            srcBottom = std::clamp(srcBottom, 0, m_screen->rowCount() - 1);
            srcRight  = std::clamp(srcRight, 0, m_screen->columnCount() - 1);
            dstTop    = std::clamp(dstTop, 0, m_screen->rowCount() - 1);
            dstLeft   = std::clamp(dstLeft, 0, m_screen->columnCount() - 1);

            // Ensure proper ordering of source rectangle
            if (srcTop > srcBottom)
                std::swap(srcTop, srcBottom);

            if (srcLeft > srcRight)
                std::swap(srcLeft, srcRight);

            // Calculate dimensions
            int width = srcRight - srcLeft + 1;
            int height = srcBottom - srcTop + 1;

            // Ensure destination fits within screen bounds
            int dstBottom = std::min(dstTop + height - 1, m_screen->rowCount() - 1);
            int dstRight  = std::min(dstLeft + width - 1, m_screen->columnCount() - 1);

            // Copy the rectangular area
            for (int row = 0; row < height && (dstTop + row) <= dstBottom; ++row) {
                for (int col = 0; col < width && (dstLeft + col) <= dstRight; ++col) {
                    if (auto *srcCell = m_screen->cellAt(srcTop + row, srcLeft + col)) {
                        if (auto *dstCell = m_screen->cellAt(dstTop + row, dstLeft + col)) {
                            *dstCell = *srcCell;
                            m_screen->markCellDirty(dstTop + row, dstLeft + col);
                        }
                    }
                }
            }
            m_screen->requestRender();
        };


        // DECRARA - Reverse Attributes in Rectangular Area
        m_dispatchMap[CSI_CODE::DECRARA] = [this](std::span<const int> params) {
            int top = params.size() > 0 ? params[0] - 1 : 0;
            int left = params.size() > 1 ? params[1] - 1 : 0;
            int bottom = params.size() > 2 ? params[2] - 1 : m_screen->rowCount() - 1;
            int right = params.size() > 3 ? params[3] - 1 : m_screen->columnCount() - 1;

            top = std::clamp(top, 0, m_screen->rowCount() - 1);
            left = std::clamp(left, 0, m_screen->columnCount() - 1);
            bottom = std::clamp(bottom, 0, m_screen->rowCount() - 1);
            right = std::clamp(right, 0, m_screen->columnCount() - 1);

            if (top > bottom) std::swap(top, bottom);
            if (left > right) std::swap(left, right);

            for (int row = top; row <= bottom; ++row) {
                for (int col = left; col <= right; ++col) {
                    if (auto *cell = m_screen->cellAt(row, col)) {
                        const auto &origAttr = cell->attributes();
                        if (origAttr) {
                            Attributes modified = *origAttr;  // Copy
                            modified.inverse = !modified.inverse;         // Modify
                            cell->setAttributes(*Attributes::intern(modified)); // Re-intern
                            cell->setDirty(true);
                            m_screen->markCellDirty(row, col);
                        }
                    }
                }
            }

            m_screen->requestRender();
        };
    }
};

}
